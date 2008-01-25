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
 * Device driver for the real card reader.
 */
public class PlatformCardDevice extends CardDevice {
    
    /**
     * Initializes the device.
     * 
     * @throws IOException If a device initialization failed.
     * @throws CardDeviceException If a configuration initialization failed.
     */
    public void init() throws IOException, CardDeviceException {
        
        if (!init0()) {
            String err_msg = getErrorMessage0();
            if (err_msg != null)
                throw new IOException(err_msg);
            else
                throw new IOException("Init failed");
        }
    }
    
    /**
     * Closes the device.
     * No exceptions thrown to avoid mess in exception handlers trying
     * to clean up and close the device.
     */
    public void close() {
        finalize0();
    }
    
    /**
     * Performs platform lock of the device. This is intended to make
     * sure that no other native application
     * uses the same device during a transaction.
     *
     * @throws IOException If a device locking failed.
     */
    public void lock() throws IOException {
        
        if (!lock0()) {
            String err_msg = getErrorMessage0();
            if (err_msg != null)
                throw new IOException(err_msg);
            else
                throw new IOException("Lock failed");
        }
    }

    /**
     * Unlocks the device.
     *
     * @throws IOException If a device unlocking failed.
     */
    public void unlock() throws IOException {
        
        if (!unlock0()) {
            String err_msg = getErrorMessage0();
            if (err_msg != null)
                throw new IOException(err_msg);
            else
                throw new IOException("Unlock failed");
        }
    }
    
    /**
     * Gets number of slots on a device. 
     *
     * @return Number of slots on a device. In case of error
     * returns 0.
     */
    public int getSlotCount() { 
        return getSlotCount0();
    }

    /**
     * Checks if this slot is SAT slot. 
     *
     * @param slotNumber Slot number
     * @return <code>true</code> if the slot is dedicated for SAT,
     *         <code>false</code> otherwise
     * @throws IOException If an error occured.
     */
    public boolean isSatSlot(int slotNumber) throws IOException { 
        int result = isSatSlot0(slotNumber);
        if (result == -1) {
            String err_msg = getErrorMessage0();
            if (err_msg != null)
                throw new IOException(err_msg);
            else
                throw new IOException("isSAT failed");
        }
        return result != 0; 
    }

    /**
     * Selects the current slot for the subsequent transfer operations.
     * For the one-slot devices the default slot number is 0.
     *
     * @param slot Slot number
     * @throws IOException If a slot selection failed.
     */
    public void selectSlot(int slot) throws IOException {
        
        if (!selectSlot0(slot)) {
            String err_msg = getErrorMessage0();
            if (err_msg != null)
                throw new IOException(err_msg);
            else
                throw new IOException("SelectSlot failed");
        }
    }
    
    /**
     * Performs reset of device.
     *
     * @param atr ATR bytes
     * @return Length of ATR
     * @throws IOException If a reset failed.
     */
    public int cmdReset(byte[] atr) 
        throws IOException {
        int bytes_read;
                
        if ((bytes_read = reset0(atr)) < 0) {
            String err_msg = getErrorMessage0();
            if (err_msg != null)
                throw new IOException(err_msg);
            else
                throw new IOException("reset failed");
        }
        return bytes_read;
    }
    
    /**
     * Performs data transfer to the device.
     *
     * @param request Request bytes
     * @param response Response bytes
     * @return Length of response
     * @throws IOException If a data transfer failed.
     */
    public int cmdXfer(byte[] request, byte[] response) 
        throws IOException {
        int bytes_read;
        
        if ((bytes_read = cmdXfer0(request, response)) < 0) {
            String err_msg = getErrorMessage0();
            if (err_msg != null)
                throw new IOException(err_msg);
            else
                throw new IOException("cmdXfer failed");
        }
        return bytes_read;
    }
    
    /**
     * Checks if the card in the selected slot was changed 
     * since last call or since last reset.
     * Always called after any transfer operation, so
     * implementation can check and store that status during 
     * this operation.
     *
     * @return true if was changed, false otherwise.
     * @throws IOException If something fails.
     */
    public boolean isCardChanged() throws IOException {
        int result = checkCardMovement0();
        if (result < 0) {
            String err_msg = getErrorMessage0();
            if (err_msg != null)
                throw new IOException(err_msg);
            else
                throw new IOException("Checking of card movement failed");
        }
        if (result == 0) {
            return false;
        } else {
            return true;
        }
    }
    

    /**
     * Initializes the device.
     * 
     * @return true if successful, false otherwise
     * @throws CardDeviceException If a configuration failed.
     */
    private native boolean init0() throws CardDeviceException;
    
    /**
     * Finalizes the device.
     */
    private native void finalize0();
    /**
     * Locks the device.
     * 
     * @return true if successful, false otherwise
     */
    private native boolean lock0();
    
    /**
     * Unlocks the device.
     * 
     * @return true if successful, false otherwise
     */
    private native boolean unlock0();
    
    /**
     * Gets number of slots on a device. 
     *
     * @return Number of slots on a device. In case of error
     * returns 0.
     */
    private native int getSlotCount0();

    /**
     * Checks if this slot is SAT slot. 
     *
     * @param slotNumber Slot number
     * @return <code> 1</code> if the slot is dedicated for SAT,
     *         <code> 0</code> if not,
     *         <code>-1</code> if any error occured
     */
    private native int isSatSlot0(int slotNumber);

    /**
     * Selects the current slot for the subsequent transfer operations.
     * For the one-slot devices the default slot number is 0.
     *
     * @param slotIndex Slot number
     * @return true if successful, false otherwise
     */
    private native boolean selectSlot0(int slotIndex);
    
    /**
     * Performs reset of device.
     * Fills diven array with ATR bytes.
     *
     * @param atr ATR bytes
     * @return Length of ATR if successful, < 0 otherwise
     */
    private native int reset0(byte[] atr);
    
    /**
     * Performs data transfer to the device.
     *
     * @param request Request bytes
     * @param response Response bytes
     * @return Length of response if successful, < 0 otherwise
     */
    private native int cmdXfer0(byte[] request, byte[] response);
    
    /**
     * Checks if the card in the selected slot was changed 
     * since last call or since last reset.
     * Called after any transfer operation, so
     * implementation can check and store that status during 
     * this operation.
     *
     * @return 0 if no movements has happened, 
     *         > 0 if the card was changed,
     *         < 0 if an error occured.
     */
    private native int checkCardMovement0();
    
    /**
     * Retrives last error message.
     *
     * @return Error message as String
     */
    private native String getErrorMessage0();
}

