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
 
package com.sun.cardreader;

import java.io.IOException;

/**
 * This class represents card device abstraction.
 */

public abstract class CardDevice {
    /**
     * Number of start slot of the device. The slot factory holds
     * a table with global slot numbers. Every device operates with
     * local ones (numbers inside the device). 
     * <code>startSlotNumber</code> is global number of first
     * local slot.
     */
    private int startSlotNumber;
    
    /** Default SAT slot number. */
    private final static int DEFAULT_SAT_SLOT = 0;

    /**
     * Answer-To-Reset bytes receiving during last reset.
     */
    private byte[] ATR;
    
    /**
     * Initializes the device.
     * 
     * @throws IOException If a device initialization failed.
     * @throws CardDeviceException If a device configuration is bad.
     */
    public abstract void init() throws IOException, CardDeviceException;
    
    /**
     * Closes the device.
     * No exceptions thrown to avoid mess in exception handlers trying
     * to clean up and close the device.
     */
    public abstract void close();
    
    /**
     * Performs platform lock of the device. This is intended to make
     * sure that no other native application
     * uses the same device during a transaction.
     *
     * @throws IOException If a device locking failed.
     */
    public abstract void lock() throws IOException;
    
    /**
     * Unlocks the device.
     *
     * @throws IOException If a device unlocking failed.
     */
    public abstract void unlock() throws IOException;

    /**
     * Open the specified slot. This is for situations when slot creation
     * requires additional actions. By default doing nothing but this is
     * not an abstract method because most devices don't need it.
     *
     * @param slot Slot number
     * @throws IOException If slot opening failed.
     */
    public void openSlot(int slot) throws IOException {
    }
    
    /**
     * Close the specified slot. This is for situations when slot closure
     * requires additional actions. By default doing nothing but this is
     * not an abstract method becuase most devices don't need it.
     *
     * @param slot Slot number
     * @throws IOException If slot close failed.
     */
    public void closeSlot(int slot) throws IOException {
    }
    
    /**
     * Selects the current slot for the subsequent transfer operations.
     * For the one-slot devices the default slot number is 0.
     *
     * @param slot Slot number
     * @throws IOException If a slot selection failed.
     */
    public abstract void selectSlot(int slot) throws IOException;

    /**
     * Gets number of slots on a device. Default implementation returns 1 which
     * is ok for most devices.
     *
     * @return Number of slots on a device
     */
    public int getSlotCount() { 
        return 1; 
    }

    /**
     * Checks if this slot is SAT slot. Default implementation returns true
     * if <code>slotNumber</code> is 0.
     *
     * @param slotNumber Slot number
     * @return <code>true</code> if the slot is dedicated for SAT,
     *         <code>false</code> otherwise
     * @throws IOException If an error occured.
     */
    public boolean isSatSlot(int slotNumber) throws IOException { 
        return slotNumber == DEFAULT_SAT_SLOT; 
    }

    /**
     * Resets the device. Wrapper method for 
     * <code>cmdReset</code>.
     *
     * @throws IOException If a reset failed.
     */
    public void reset() throws IOException {
        int bytes_read;
        byte[] r = new byte[255];
        
        bytes_read = cmdReset(r);
        if (bytes_read > 0) {
            ATR = new byte[bytes_read];
            System.arraycopy(r, 0, ATR, 0, bytes_read);
        } else
            throw new IOException("Empty ATR");
    }
	
    /**
     * Performs reset of device.
     * Returns ATR into provided buffer
     *
     * @param atr Buffer for ATR bytes
     * @return Length of ATR
     * @throws IOException If a reset failed.
     */
    protected abstract int cmdReset(byte[] atr) throws IOException;
	
    /**
     * Performs 'POWER DOWN' command. By default does nothing.
     *
     * @throws IOException If command failed.
     */
    public void cmdPowerDown() throws IOException {
    }

    /**
     * Gets Answer-To-Reset bytes from the device.
     *
     * @return ATR bytes
     * @throws IOException If data transfer failed.
     */
    public byte[] getATR() throws IOException {
        return ATR;
    }

    /**
     * Performs data transfer to the device. Wrapper
     * method for <code>cmdXfer</code>.
     *
     * @param request Request bytes
     * @param response Response bytes
     * @return Length of response
     * @throws IOException If a data transfer failed.
     */
    public int xfer(byte[] request, byte[] response) throws IOException {
        return cmdXfer(request, response);
    }

    /**
     * Performs data transfer to the device.
     *
     * @param request Request bytes
     * @param response Response bytes
     * @return Length of response
     * @throws IOException If a data transfer failed.
     */
    protected abstract int cmdXfer(byte[] request, byte[] response) 
	throws IOException;

    /**
     * Checks if the card in the selected slot was changed 
     * since last call or since last reset.
     * Called after any transfer operation, so
     * implementation can check and store that status during 
     * this operation.
     *
     * @return true if was changed, false otherwise.
     * @throws IOException If something fails.
     */
    public abstract boolean isCardChanged() throws IOException;
    
    /**
     * Gets start slot number in the global slot numbering scheme.
     *
     * @return Number of the first device slot in the global numbering scheme
     */
    public int getStartSlotNumber() {
        return this.startSlotNumber; 
    }

    /**
     * Stores start slot number.
     *
     * @param slot Number of the first device slot in the global 
     *             numbering scheme
     */
    public void setStartSlotNumber(int slot) {
        this.startSlotNumber = slot;
    }

    /**
     * Checks if the global slot number belongs to the device.
     *
     * @param slot Global slot number
     * @return True if slot belongs to this device, otherwise false
     */
    public boolean checkSlotNumber(int slot) {
        return ((slot >= this.startSlotNumber) && 
                (slot < this.startSlotNumber + getSlotCount()));
    }
}
