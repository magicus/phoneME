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
import com.sun.cardreader.*;
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
    
    /** Contains references to all CADs or slots. */
    static Cad[] cads;

    /** Objects used to synchronize access to CADs. */
    private static Object[] sync;
    
    /** Saved handle of open SAT connection. */
    private static Handle satHandle = null;
    
    /**
     * This method is called when there is SAT connection creation in 
     * progress. This method checks if the the required CAD objects have
     * been created or not and creates them. It then checks if the SIM
     * application has been selected and can be communicated with.
     * If it is not present, this method throws ConnectionNotFound
     * exception. If SAT connection is in use this method throws
     * IOException.
     *
     * @param slot Slot number
     * @return new connection handle
     * @exception IOException when SIM is not present or
     * connection cannot be established with the card.
     */
    static Handle openSATConnection(int slot)
        throws IOException {
        
        Handle h = null;
        
        init();
        
        /*
         * IMPL_NOTE: SATSA spec does not require checking for existence.
         * JDTS test does not close old connection and tries open new one.
         */
if (false) { 
        // Check if old SAT connection still alive
        if (satHandle != null &&
                satHandle.opened &&
                satHandle.cad.getCardSessionId() != -1 &&
                satHandle.cardSessionId == satHandle.cad.getCardSessionId()) {
            throw new IOException("SAT already open");
        }
}
        synchronized (sync[slot]) {
            try {
                if (cads[slot] == null) {
                    cads[slot] = new Cad(slot);
                }
            } catch (IOException e) {
                freeResources(slot);
                throw new ConnectionNotFoundException("" + e);
            }
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
                    try {
                        h = new Handle(slot,
                            cads[slot].selectApplication(true, apdu));
                    } catch (IOException e) {
                        if (cads[slot].isAlive()) {
                            cads[slot].clean();
                            throw e;
                        }
                        // at this point we need to close socket
                        freeResources(slot);
                        throw e;
                    }
                }
            }
            if (h == null) {
                if (!cads[slot].isAlive()) {
                    throw new ConnectionNotFoundException("SIM not found");
                }
                h = new Handle(slot, 0);
            }
            satHandle = h; // Save handle created for SAT connection
            return h;
        }
    }
    
    
    /**
     * Opens a connection to a smart card for reading of ACL.
     * This method is called from <code>initACL</code>
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
        
        Handle h = null;
        
        init();
        
        try {
            if (cads[slot] == null) {
                cads[slot] = new Cad(slot);
            }
        } catch (IOException e) {
            freeResources(slot);
            throw new ConnectionNotFoundException("" + e);
        }
        try {
            return new Handle(slot,
                   cads[slot].selectApplication(false, apdu));
        } catch (IOException e) {
            if (cads[slot].isAlive()) {
                cads[slot].clean();
                throw e;
            }
            // at this point we need to close slot
            freeResources(slot);
            throw e;
        }
    }
    
    /**
     * This method is called when there is a connection creation is in 
     * progress
     * and specifically card application selection is required. This method
     * checks if the the required CAD objects have been created or not and
     * creates them. It then calls the selectApplication method on the CAD to
     * select the application. If the card application selection is successful
     * this method gets the channel information from the CAD which it returns
     * to the APDUConnection object.
     *
     * @param selectAPDU byte encoded selection APDU
     * @param slot contains the information regarding the slot the has
     * the card containing the application with which connection needs
     * to be established
     * @return new connection handle
     * @exception IOException when selection is not successful
     */
    public static Handle selectApplication(byte[] selectAPDU, int slot)
                     throws IOException {

        init();
        synchronized (sync[slot]) {
            try {
                if (cads[slot] == null) {
                    cads[slot] = new Cad(slot);
                }
            } catch (IOException e) {
                freeResources(slot);
                throw new ConnectionNotFoundException("" + e);
            }
            try {
                return new Handle(slot,
                        cads[slot].selectApplication(false, selectAPDU));
            } catch (IOException e) {
                if (cads[slot].isAlive()) {
                    cads[slot].clean();
                    throw e;
                }
                // at this point we need to close slot
                freeResources(slot);
                throw e;
            }
        }
    }

    /**
     * Initializes ACL for the slot.
     * @param slot The slot number
     */
    public static void initACL(int slot) {
        try {
            init();
            synchronized (sync[slot]) {
                if (cads[slot] == null) {
                    cads[slot] = new Cad(slot);
                }
                cads[slot].initACL();
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
                "Invalid configuration");
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
        init();
        return SlotFactory.isSatSlot(slot);
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
        return cads.length;
    }


    /**
     * This method reads the configuration file for number of slots
     * and their parameters and performs necessary initialization.
     * @exception IOException if there are any config problem
     */
    synchronized private static void init() throws IOException {

        if (cads != null) {
            return;
        }
        try {
            SlotFactory.init();
        } catch (CardDeviceException e) {
            if (e.getMessage().equals("stub")) {
                throw new ConnectionNotFoundException("Null implementation: " +
                        "J2ME device does not have the smart card slot");
            } else {                
                throw new IOException("Config error: " + e.getMessage());
            }
        }
        int slots = SlotFactory.getCardSlotCount();

        sync = new Object[slots];
        for (int i = 0; i < slots; i++) {
            sync[i] = new Object();
        }

        cads = new Cad[slots];
    }

    /**
     * This method takes in the command APDU in the form of a byte array, 
     * converts it into an Apdu object which facilitates the processing 
     * of APDU data and then calls the exchangeAPDU method in CadClient 
     * to send the APDU to the card. If there are no errors, this method
     * gets the response APDU data from the apdu object and returns that.
     * @param h connection handle
     * @param apduData APDU data in byte array form
     * @return response APDU data in byte array form
     * @exception IOException if there are any IO problems
     */
    public static byte[] exchangeAPDU(Handle h, byte[] apduData)
            throws IOException {

        synchronized (sync[h.slot]) {

            if (cads[h.slot] == null) {
                try {
                    cads[h.slot] = new Cad(h.slot);
                } catch (IOException e) {
                    throw e;
                }
            }

            if (h.getCardSessionId() == -1) {
                throw new IOException("Card removed");
            }
            if (h.cardSessionId != h.getCardSessionId()) {
                throw new InterruptedIOException();
            }
            if (!h.opened) {
                throw new InterruptedIOException("Connection closed");
            }

            try {

                byte[] response = h.cad.exchangeApdu(h, apduData);

                if (!h.opened) {
                    throw new InterruptedIOException("Connection closed");
                }
                return response;
            } catch (InterruptedIOException ie) {
                throw ie;
            } catch (IOException e) {
                freeResources(h.slot);
                throw e;
            }
        }
    }
    
    /**
     * Closes the connection.
     * @param h connection handle
     * @exception IOException if there are any IO problems
     */
    public static void closeConnection(Handle h) throws IOException {

        h.opened = false;
        synchronized (sync[h.slot]) {
            if (h.cardSessionId != h.getCardSessionId()) {
                throw new IOException();
            }
            h.cad.closeChannel(h.channel);
        }
    }
    
    /**
     * Frees up connection resources if connection was closed because
     * of an error.
     * @param slotNumber the selected slot
     */
    private static void freeResources(int slotNumber) {
    	if (cads[slotNumber] != null) {
                cads[slotNumber].freeSystemResources(); 
    	}
    	cads[slotNumber] = null;
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
}
