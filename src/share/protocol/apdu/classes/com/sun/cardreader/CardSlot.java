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
import java.io.InterruptedIOException;

/**
 * This class represents card slot abstraction.
 */
public class CardSlot {
    /**
     * Device object.
     */
    private CardDevice device;

    /**
     * Local slot number (inside the device).
     */
    private int slotNumber;

    /**
     * Indicates that the slot has been opened.
     */
    private boolean opened;

    /** Unique card session identifier. */
    private int cardSessionId;

     /** This field is used to assign unique card session identifiers. */
    private static int sessionId = 1;

    /**
     * ATR bytes kept since last reset.
     */
    private byte[] ATR;

    /**
     * A flag which shows if the card was changed.
     */
    private boolean cardChanged;

    /**
     * isAlive command APDU.
     */
    private byte[] isAliveAPDU;

    /**
     * isAlive command response buffer.
     */
    private byte[] isAliveResponse;

    /**
     * Creates card slot on the specified device.
     *
     * @param device Card device for which the slot is created
     * @param slot Local device slot number
     * @throws IOException If slot creation failed.
     */
    public CardSlot(CardDevice device, int slot)
	throws IOException {
        this.device = device;
        this.slotNumber = slot;
        opened = false;
        cardChanged = true;
        isAliveAPDU = new byte[] {0, 0x70, (byte) 0x80, 0};
        isAliveResponse = new byte[2];
    }

    /**
     * Performs data transfer to the device slot.
     * It must be called where slot is locked.
     *
     * @param request Request APDU bytes
     * @param response Response bytes
     * @return Length of response
     * @throws IOException If a data transfer failed or
     *         the card was changed.
     */
    public int xferData(byte[] request, byte[] response)
            throws IOException {
        return xferData(false, request, response);
    }

    /**
     * Performs data transfer to the device slot.
     * It must be called where slot is locked.
     *
     * @param aliveChecking true if isAlive command is performed,
     *                      false otherwise
     * @param request Request APDU bytes
     * @param response Response bytes
     * @return Length of response
     * @throws IOException If a data transfer failed or
     *         the card was changed.
     */
    private int xferData(boolean aliveChecking,
            byte[] request, byte[] response)
            throws IOException {
        int bytes_read;

        if (isCardChanged() && !aliveChecking) {
            unlockSlot();
            throw new InterruptedIOException("Card changed");
        }

        try {
            bytes_read = device.xfer(request, response);
        }
        catch (IOException e) {
            if (isCardChanged() && !aliveChecking) {
                unlockSlot();
                throw new InterruptedIOException("Card changed: " + e);
            }
            unlockSlot();
            throw e;
        }
        catch (RuntimeException e) { // We must unlock slot
            unlockSlot();
            throw e;
        }
        if (isCardChanged() && !aliveChecking) {
            unlockSlot();
            throw new InterruptedIOException("Card changed");
        }
        return bytes_read;
    }

    /**
     * Checks if the the connection is still live or not. It
     * is a public wrapper for <code>doIsAlive</code>. Also
     * checks if a card was changed. Slot must not be locked.
     * @return <code>true</code> if the connection is alive
     */
    public boolean isAlive() {
        boolean alive = doIsAlive();
        if (!alive) {
            try {
                lockSlot();
                boolean changed = isCardChanged();
                unlockSlot();
                if (changed) {
                    alive = doIsAlive();
                }                
            } catch (IOException e) {
                alive = false;
            }
        }
        return alive;
    }

    /**
     * Checks if the the connection is still live or not by trying to
     * send an APDU to close channel 0. If we get an IOException back
     * that means a card or cref is gone.
     * @return <code>true</code> if the connection is alive
     */
    private boolean doIsAlive() {
        try {
            lockSlot();
            xferData(true, isAliveAPDU, isAliveResponse);
            unlockSlot();
            return true;
        } catch (IOException e) {
            return false;
        }
    }

    /**
     * Gets ATR bytes from the device.
     *
     * @return ATR bytes.
     */
    public byte[] getATR() {
        byte[] result;

        if (opened && ATR.length > 0) {
            result = new byte[ATR.length];
            System.arraycopy(ATR, 0, result, 0, ATR.length);
        } else {
            result = null;
        }
        return result;
    }

    /**
     * Gets local device slot number.
     *
     * @return Local slot number.
     */
    public int getSlotNumber() {
        return slotNumber;
    }

    /**
     * Returns the card session identifier.
     * This number is different for different card sessions.
     * @return the card session identifier or -1 when slot is closed
     */
    public int getCardSessionId() {
        return (opened && !cardChanged) ? cardSessionId : -1;
    }

    /**
     * Starts operations with the slot.
     * All the transfer operations (<code>xferData</code>)
     * should be performed when the device is locked.
     * If any problem occurs an unlock must be done.
     *
     * @throws IOException If something fails.
     */
    public void lockSlot() throws IOException {
        try {
            device.lock();
            open();
            device.selectSlot(slotNumber);
        }
        catch (IOException e) {
            unlockSlot();
            throw e;
        }
    }

    /**
     * Ends operations with the slot.
     *
     * IOException is ignored.
     */
    public void unlockSlot() {
        try {
            device.unlock();
        }
        catch (IOException e) {}
    }

    /**
     * Makes sure that slot is ready for connection.
     *
     * @return true if this is first connection to inserted card
     * @throws IOException If something fails.
     */
    public boolean initConnection() throws IOException {
        boolean prevCardChanged;
        lockSlot();
        prevCardChanged = isCardChanged();
        unlockSlot();
        cardChanged = false;
        return prevCardChanged;
    }

    /**
     * Initializes ACL for the slot.
     */
    public void initACL() {
        boolean changed;
        
        /* Make CARD_CHANGED event occur if new cref connection was opened */
        doIsAlive();
        try {
            lockSlot();
            changed = isCardChanged();
            unlockSlot();
            if (changed) {
                com.sun.satsa.acl.AccessControlManager.init(slotNumber);
            }
        } catch (IOException e) {} // ignored
    }

    /**
     * Closes the slot in case of error.
     * <code>device.closeSlot</code> usually
     * does nothing.
     * Used for socket-like 'devices'.
     *
     * @throws IOException If something fails.
     */
    public void closeSlot() throws IOException {
        if (opened) {
            device.closeSlot(slotNumber);
        }
        opened = false;
    }

    /**
     * Gets device of this slot. For using in derived classes.
     *
     * @return Slot device
     */
    protected CardDevice getDevice() {
        return this.device;
    }

    /**
     * Checks if this slot is SAT slot.
     * @return SAT check result
     * @throws IOException If an error occured.
     */
    public boolean isSAT() throws IOException {
        lockSlot();
        boolean isSatSlot = device.isSatSlot(slotNumber);
        unlockSlot();

        return isSatSlot;
    }

    /**
     * Checks if the card was withdrawn or inserted.
     * The device must be locked.
     *
     * @return true if a card was changed, false otherwise
     * @throws IOException If something wrong.
     */
    private boolean isCardChanged() throws IOException {
        try {
            if (device.isCardChanged()) {
                closeSlot();
            }
            open();
        } catch (IOException e) {
            unlockSlot();
            throw e;
        }
        if (cardChanged) {
            return true;
        }
        return false;
    }

    /**
     * Tries to open slot if it closed. Performes the 'RESET' command.
     * The device must be locked.
     *
     * @throws IOException If open fails.
     */
    private void open() throws IOException {
        boolean openedSuccessfuly = false;
        if (!opened) {
            try {
                device.openSlot(slotNumber);
                openedSuccessfuly = true;
                device.selectSlot(slotNumber);
                device.reset();
                cardSessionId = sessionId++;
            } catch (IOException ie) {
                if (openedSuccessfuly) {
                    try {
                        device.closeSlot(slotNumber);
                    } catch (IOException ign) {} // ignored
                }
                throw new IOException("Cannot open slot #" +
                            slotNumber + ": " + ie);
            }
            ATR = device.getATR();
            cardChanged = true;
            opened = true;
        }
    }
}
