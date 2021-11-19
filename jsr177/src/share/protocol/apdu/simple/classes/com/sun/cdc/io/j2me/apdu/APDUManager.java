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

import javax.microedition.io.*;
import com.sun.cardreader.CardDeviceException;
import com.sun.j2me.main.Configuration;

import java.io.*;

/**
 * This class provides an interface to the low level APDU protocol 
 * details as described below:
 * 
 * 1. Manage Card power up/reset. 
 * 2. APDU scheduler-Can assign priorities to APDUs to synchronize card 
 * access. Synchronization is required between APDUs being sent by native 
 * telecom software and APDUs sent by J2ME application. The reference 
 * implementation will only have APDUs sent by the J2ME application which 
 * would all have the same priority.  
 * 3. APDU Dispatcher-Support APDU exchange with the card. It has transaction 
 * capabilities to receive the APDUs from applications on the consumer device 
 * and return the responses to the corresponding application. 
 * 4. J2ME applications are oblivious of the logical channel information. 
 * Getting the logical channel for communicating with the smart card 
 * application is the responsibility of the APDUManager which sends 
 * appropriate APDUs to the card to get the logical channels 
 * assigned for a new connection or close channels when the connection is 
 * closed.
 */
public class APDUManager {

    /** SAT selection APDU for testing purposes. */
    private static final String SAT_APDU_PROP = 
        "com.sun.io.j2me.apdu.satselectapdu";
    
    /** Contains references to all supported slots. */
    static Slot[] slots;
    
    /**
     * Creates SAT connection.
     *
     * @param slot Slot number
     * @return new connection handle
     * @exception IOException when SIM is not present or
     * connection cannot be established with the card.
     */
    static Handle openSATConnection(int slot)
        throws IOException {
        
        Handle h = null;
        
        checkSlotNumber(slot);
        Slot cardSlot = slots[slot];
        
        String satAPDU = Configuration.getProperty(SAT_APDU_PROP);
        if (satAPDU != null) {
            byte[] apdu = new byte[24];
            boolean ok;
            try {
                int len = parseDottedBytes(satAPDU, apdu, 0);
                ok = len >= 10 && len <= 22; // 5 bytes hdr + AID + Le
                int Lc = (apdu[4] & 0xFF);
                if (ok && (len < Lc + 5 || 
                           len > Lc + 5 + 1 ||
                           apdu[0] != (byte)0x00 ||
                           apdu[1] != (byte)0xA4 ||
                           apdu[2] != (byte)0x04)) {
                    ok = false;
                }
                if (ok && len == Lc + 5) {
                    apdu[len] = 0x7F;
                }
            } catch (NullPointerException npe) {
                ok = false;
            } catch (IndexOutOfBoundsException iobe) {
                ok = false;
            } catch (IllegalArgumentException iae) {
                ok = false;
            }

            if (ok) {
                h = selectApplication(true, apdu, slot);
            }
        }
        if (h == null) {
            if (!isAlive(cardSlot)) {
                throw new ConnectionNotFoundException("SIM not found");
            }
            h = new Handle(slot, 0);
        }
        return h;
    }
    
    
    /**
     * Opens a connection to a smart card for reading of ACL.
     * This method is called from <code>reset</code>
     * method, so it does not need <code>synchronized</code>
     * statement.
     *
     * @param apdu The APDU that will be used for opening
     * @param slot Slot number
     * @return new connection handle
     * @exception IOException when a card is not present or
     * connection cannot be established with the card.
     */
    public static Handle openACLConnection(byte[] apdu, int slot)
        throws IOException {
        
        checkSlotNumber(slot);
        return selectApplication(false, apdu, slot);
    }
    
    /**
     * Initializes ACL for the slot (if needed). This method is invoked
     * when an establishment of new connection is being performed. 
     * @param slot The slot number
     */
    public static void initACL(int slot) {
        
        try {
            checkSlotNumber(slot);
            Slot cardSlot = slots[slot];
            if (!cardSlot.powered) {
                reset(cardSlot); // here a reading of ACL is being performed
            }
        } catch (IOException e) {} // ignored
    }

    /**
     * Verifies that slot number is correct. Invokes init method
     * if necessary.
     * @param slot the slot number
     * @throws ConnectionNotFoundException if slot number is wrong
     */
    public static void checkSlotNumber(int slot) throws
            ConnectionNotFoundException {

        try {
            init();
        } catch (ConnectionNotFoundException cnfe) {
            throw cnfe;
        } catch (IOException e) {
            throw new ConnectionNotFoundException(
                "Invalid configuration: " + e);
        }
        if (slot < 0 || slot >= getSlotCount()) {
            throw new ConnectionNotFoundException(
                    "Invalid slot identifier: " + slot);
        }
    }
    
    /**
     * Checks if this slot is SAT slot.
     * @param slot the slot number
     * @return SAT check result
     * @throws IOException If an error occured.
     */
    public static boolean isSatSlot(int slot) throws IOException {
        checkSlotNumber(slot);
        return isSAT(slot);
    }

    /**
     * This method returns the ATR received from the card.
     * @param slot the slot number
     * @return ATR information received from the card at startup or
     * reset. In case of I/O troubles returns null.
     */
    public static byte[] getATR(int slot) {
        byte[] result = null;
        Slot cardSlot = slots[slot];
        
        if (isAlive(cardSlot) && cardSlot.atr != null) {
            int len = cardSlot.atr.length;
            result = new byte[len];
            System.arraycopy(cardSlot.atr, 0, result, 0, len);
        }
        return result;
    }

    /**
     * Returns the number of slots.
     * @return the number of slots. If error occured it returns 0.
     */
    public static int getSlotCount() {
        try {
            init();
        }
        catch (IOException e) {
            return 0;
        }
        return slots.length;
    }


    /**
     * Checks if the the connection is still live or not. It sends special
     * <code>isAliveAPDU</code> to the card and checks if an error occured.
     *
     * @param slot The slot object.
     * @return <code>true</code> if the connection is alive, <false> otherwise
     */
    private static boolean isAlive(Slot slot) {
        int tries = 2;
        do {
            if (!slot.powered) {
                try {
                    clean(slot);
                    reset(slot);
                } catch (IOException e) {} // ignored
            }
            try {
                exchangeAPDU(slot, slot.isAliveAPDU);
                return true;
            } catch (IOException e) {} // ignored
        } while (--tries > 0);
        return false;
    }
    
    /**
     * Cleans the slot object before a reset.
     *
     * @param slot The slot object.
     */
    private static void clean(Slot slot) {
        slot.basicChannelInUse = false;
        slot.SIMPresent = false;
        slot.FCI = null;
    }

    /**
     * This method is used to close connection with the card. If the
     * channel number passed to this method is for basic channel then
     * the basic channel is marked as available and nothing is
     * communicated with the card.
     * If the channel is not the basic channel, a request to close the
     * channel is sent to the card.
     * @param slot the slot object
     * @param channel channel number
     * @exception IOException if a communication error happens
     */
    private static void closeChannel(Slot slot, int channel) 
            throws IOException {
        
        if (channel == 0) {
            slot.basicChannelInUse = false;
            return;
        }

        try {
            slot.closeChannelAPDU[3] = (byte) channel;
            exchangeAPDU(slot, slot.closeChannelAPDU);
        } catch (IOException ioException) {
            throw new IOException("Error closing connection: " + ioException);
        }
    }

    /**
     * The public method which should be called when application
     * selection is required. Calls an internal method.
     *
     * @param selectAPDU byte encoded selection APDU
     * @param slot slot number
     * @return new connection handle
     * @exception IOException when selection is not successful
     */
    public static Handle selectApplication(byte[] selectAPDU, int slot)
                  throws IOException {
        checkSlotNumber(slot);
        return selectApplication(false, selectAPDU, slot);
    }
     
    /**
     * This method is called when there is a connection creation is in 
     * progress
     * and specifically card application selection is required. If the card 
     * application selection is successful
     * this method gets the channel information from the CAD which it returns
     * to the APDUConnection object.
     *
     * @param forSAT is this selection is making for SAT?
     * @param selectAPDU byte encoded selection APDU
     * @param slot slot number
     * @return new connection handle
     * @exception IOException when selection is not successful
     */
    private static Handle selectApplication(boolean forSAT,
                  byte[] selectAPDU, int slot) throws IOException {

        int channel;
        
        Slot cardSlot = slots[slot]; // we have checked slot number earlier

        // Test if 'POWER UP' is needed or a card was changed
        if (!isAlive(cardSlot)) {
            throw new ConnectionNotFoundException("SmartCard not found");
        }

        if (!forSAT && (cardSlot.basicChannelInUse || cardSlot.SIMPresent)) {
            // get another channel for communication.
            byte[] response = exchangeAPDU(cardSlot, cardSlot.getChannelAPDU);
            if (response.length  == 2) {
                // just got back the status word
                throw new IOException("No logical channel available");
            }
            // new channel number is in the first byte of response
            channel = response[0];
        } else {
            cardSlot.basicChannelInUse = true;
            channel = 0;
        }
        
        selectAPDU[0] = (byte)((selectAPDU[0] & 0xFC) | channel);

        byte[] result = exchangeAPDU(cardSlot, selectAPDU);

        int sw1 = result[result.length - 2] & 0xFF;
        int sw2 = result[result.length - 1] & 0xFF;
        if ((sw1 << 8) + sw2 != 0x9000) {
            closeChannel(cardSlot, channel);
            throw new ConnectionNotFoundException(
                    "Card application selection failed");
        }
        cardSlot.FCI = result;
        return new Handle(slot, channel);
    }

    /**
     * This method reads the configuration file for number of slots
     * and their parameters and performs necessary initialization.
     * @exception IOException if there are any config problem
     */
    synchronized private static void init() throws IOException {

        if (slots != null) {
            return;
        }
        
        try {
            int slotCount = init0();
        
            slots = new Slot[slotCount];
            for (int i = 0; i < slotCount; i++) {
                slots[i] = new Slot(i);
            }
        } catch (CardDeviceException e) {
            if (e.getMessage().equals("stub")) {
                throw new ConnectionNotFoundException("Null implementation: " +
                        "J2ME device does not have the smart card slot");
            } else {                
                throw new IOException("Config error: " + e.getMessage());
            }
        }
    }

    /**
     * This public method takes in the command APDU in the form of 
     * a byte array and calls the native exchangeAPDU0 method 
     * to send the APDU to the card. If there are no errors, this method
     * gets the response APDU data from the card and returns that.
     * @param h connection handle
     * @param apduData APDU data in byte array form
     * @return response APDU data in byte array form
     * @exception IOException if there are any IO problems
     */
    public static byte[] exchangeAPDU(Handle h, byte[] apduData)
            throws IOException {
        byte[] retData;
        
        Slot slot = h.cardSlot;
        try {
            synchronized (slot) {
                byte[] respBuffer = slot.respBuffer;
                int respLen = exchangeAPDU0(h, null, apduData, respBuffer);
                retData = new byte[respLen];
                System.arraycopy(respBuffer, 0, retData, 0, respLen);
            }
            return retData;
        } catch (IOException e) {
            if (!slot.powered) {
                // power up the slot after removal/insertion
                isAlive(slot);
            }
            throw e;
        }
    }
    
    /**
     * This internal method takes in the command APDU in the form of 
     * a byte array and calls the native exchangeAPDU0 method 
     * to send the APDU to the card. If there are no errors, this method
     * gets the response APDU data from the card and returns that.
     * @param slot the slot object
     * @param apduData APDU data in byte array form
     * @return response APDU data in byte array form
     * @exception IOException if there are any IO problems
     */
    private static byte[] exchangeAPDU(Slot slot, byte[] apduData)
            throws IOException {
        byte[] retData;
        synchronized (slot) {
            byte[] respBuffer = slot.respBuffer;
            int respLen = exchangeAPDU0(null, slot, apduData, respBuffer);
            retData = new byte[respLen];
            System.arraycopy(respBuffer, 0, retData, 0, respLen);
        }
        return retData;
    }
    /**
     * Performs reset of device.
     * Saves ATR into provided slot object. After the reset invokes 
     * an ACL loading for this slot.
     *
     * @param cardSlot the slot object to be reset
     * @throws IOException If a reset failed.
     */
    private static void reset(Slot cardSlot) throws IOException {
        synchronized (cardSlot) {
            byte[] atr = reset0(cardSlot);
            cardSlot.atr = atr;
            // after reset we must reload access control file
            com.sun.satsa.acl.AccessControlManager.init(cardSlot.slot);
            cardSlot.SIMPresent = isSatSlot(cardSlot.slot);
        }
    }
    
    /**
     * Closes the connection.
     * @param h connection handle
     * @exception IOException if there are any IO problems
     */
    public static void closeConnection(Handle h) throws IOException {
        Slot cardSlot = h.cardSlot;
        synchronized (cardSlot) {
            h.opened = false;
            if (h.cardSessionId != cardSlot.cardSessionId) {
                throw new IOException();
            }
            closeChannel(cardSlot, h.channel);
        }
    }
    
    /**
     * Parses string that contains hexadecimal byte values separated by
     * dots. May throw runtime exceptions.
     * @param src source string
     * @param dest destination array
     * @param offset target offset
     * @return number of bytes parsed
     */
    public static int parseDottedBytes(String src, byte[] dest,
                                       int offset) {

        int i = 0;
        int len = 0;
        int j;

        while (i != src.length() + 1) {

            if ((j = src.indexOf('.', i)) == -1) {
                j = src.length();
            }

            int l = Integer.parseInt(src.substring(i, j), 16);
            if (l != (l & 0xff)) {
                throw new IllegalArgumentException();
            }
            dest[offset + len++] = (byte) l;
            i = j + 1;
        }
        return len;
    }
    
    /**
     * Performs data transfer to the device. This method must be called within
     * <code>synchronize</code> block with the Slot object.
     *
     * @param h Connection handle. Can be null for internal purposes
     * @param slot Slot object. Unused when <code>h</code> is not null. 
     * Must be provided if <code>h</code> is <code>null</code>.
     * @param request Buffer with request data
     * @param response Buffer for response data
     * @return Length of response data
     * @exception NullPointerException if any needed parameter is null
     * @exception IllegalArgumentException if request does not contain proper 
     * APDU
     * @exception InterruptedIOException if the connection handle is suddenly 
     * closed
     * in the middle of exchange or the card was removed and inserted again
     * @exception IOException if any I/O troubles occured
     */
    private static native int exchangeAPDU0(Handle h, Slot slot, 
                          byte[] request, byte[] response) throws IOException;
    
    /**
     * Performs reset of the card in the slot. This method must be called within
     * <code>synchronize</code> block with the Slot object.
     *
     * @param slot Slot object
     * @return byte array with ATR
     * @exception NullPointerException if parameter is null
     * @exception IOException if any i/o troubles occured
     */
    private static native byte[] reset0(Slot slot) throws IOException;
    
    /**
     * Initializes the device.
     *
     * @return number of supported slots 
     * @exception CardDeviceException If configuration failed.
     * @exception IOException in case of I/O problems.
     */
    private static native int init0() throws IOException, CardDeviceException;

    /**
     * Checks if this slot is SAT slot. This method is invoked once after 
     * a reset of the card.
     * @param slot Slot number
     * @return <code>true</code> if the slot is dedicated for SAT,
     *         <code>false</code> if not
     * @exception IOException in case of error
     */
    private static native boolean isSAT(int slot);

}
