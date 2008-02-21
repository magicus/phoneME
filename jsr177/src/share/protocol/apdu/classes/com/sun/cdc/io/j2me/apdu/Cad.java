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

import java.io.IOException;
import java.io.InterruptedIOException;
import javax.microedition.io.ConnectionNotFoundException;
import com.sun.cardreader.*;
import com.sun.satsa.acl.*;

/**
 * The Cad class maintains the context for the client (terminal) side
 * of the terminal CAD connection. This class is responsible for sending
 * APDUs to the card, receiving back the responses and automatically
 * receiving a response if status '61 XX' is  received from the
 * card.
 */
class Cad {
   
    /**
     * Since at any time once channel always has to be open with the card
     * and that channel is generally the basic channel. If the basic
     * channel is not in use, the APDU manager can select the next
     * application on that channel.
     * This variable shows if basic channel is in use currently or not.
     */
    private boolean basicChannelInUse;

    /** 
     * Indicates if slot is the SAT slot and a SIM is installed on it.
     * This flag is set in Cad.selectApplication.
     * It is cleared when Cad.isAlive detects the death of connection or
     * Cad.getATR is not able to reset the card.
     */
    boolean SIMPresent;

    /** APDU for logical channel allocation. */
    private byte[] getChannelAPDU;

    /** APDU for closing a logical channel. */
    private byte[] closeChannelAPDU;

    /** APDU for GET RESPONSE command. */
    private byte[] getResponseAPDU;

    /** APDU for ISO7816-4 'case 2' command. Length is 5 bytes. */
    private byte[] case2APDU;

    /** FCI information received from selected applications. */
    private byte[] FCI;

    /** CardSlot created for Cad. */
    private CardSlot cardSlot;

    /** Stuff buffer for received data. */
    private TempByteArray resp_buffer = new TempByteArray();
    
    /** Stuff buffer for output data. */
    private TempByteArray output_buffer = new TempByteArray();

    /** Slot number */
    private int slotN;

    /**
     * Constructor for CADClient.
     * @param slot Global slot number
     * @throws IOException if there any problems in opening the socket or
     * streams or powering up the card.
     */
    Cad(int slot) throws IOException {
        
        try {
            cardSlot = SlotFactory.getCardSlot(slot);
            if (cardSlot == null) {
                throw new IOException("Slot factory could not create a slot");
            }
        }
        catch (CardDeviceException e) { 
            throw new IOException("Config error: " + e.getMessage());
        }
        slotN = slot;
        getChannelAPDU = new byte[] {0, 0x70, 0, 0, 1};
        closeChannelAPDU = new byte[] {0, 0x70, (byte) 0x80, 0};
        getResponseAPDU = new byte[] {0, (byte)0xC0, 0, 0, 0};
        case2APDU = new byte[5];
    }
    
    /**
     * Exchange an Apdu with a CAD.
     * @param h Handle of the connection.
     * @param commandAPDU APDU data in byte array form.
     * @return response APDU data in byte array form.
     * @exception InterruptedIOException if connection was closed in the
     * other thread.
     * @exception IOException if a communication error happens while
     * communicating with the CAD.
     */
    byte[] exchangeApdu(Handle h, byte[] commandAPDU) throws IOException {
        byte[] result;
        int result_length = 0;
        
        int Lc, Le;
        
        Lc = 0;
        Le = commandAPDU.length == 4 ? -1 : commandAPDU[4] & 0xFF; 

        if (commandAPDU.length > 5) {

            Lc = Le;

            if (5 + Lc > commandAPDU.length) {
                throw new IllegalArgumentException("Malformed APDU");
            }

            Le = 5 + Lc == commandAPDU.length ?
                    -1 : commandAPDU[5 + Lc] & 0xFF;
        }
        
        if (Le == 0) {
            Le = 256;
        }
        if (Le == -1) {
            Le = 0; 
        }
        
        int cla = commandAPDU[0] & 0xf0;
        int channel = cla != 0 && (cla < 0x80 || cla > 0xA0) ?
                      0 : commandAPDU[0] & 3;

        cardSlot.lockSlot();
        if (Lc == 0 && commandAPDU.length > 5) { // (case 4 & Lc==0) ==> case 2
            System.arraycopy(commandAPDU, 0, case2APDU, 0, 4);
            case2APDU[4] = commandAPDU[5];
            commandAPDU = case2APDU;
        }
        while (true) {
            int bytes_read;
            int sw1, sw2;
            
            bytes_read = 
                cardSlot.xferData(commandAPDU, resp_buffer.ensure(Le + 2));
            if (bytes_read < 2) {
                cardSlot.unlockSlot();
                throw new IOException("Bad response");
            }
            if (h != null && !h.opened) {
                cardSlot.unlockSlot();
                throw new InterruptedIOException("Connection closed");
            }
            sw1 = resp_buffer.val(bytes_read - 2) & 0xFF;
            sw2 = resp_buffer.val(bytes_read - 1) & 0xFF;
            
            // Correct wrong Le
             /* IMPL_NOTE: We shell make sure that Le!=sw2 but that is out of spec */
            if (bytes_read == 2 &&  
                    sw1 == 0x6C && sw2 != 0x00 &&
                    Le != 0) {
                
                Le = sw2;
                if (commandAPDU.length == 4) { // case 1
                    System.arraycopy(commandAPDU, 0, case2APDU, 0, 
                                     commandAPDU.length);
                    case2APDU[4] = (byte)(Le & 0xFF);
                    commandAPDU = case2APDU;
                } else 
                if (commandAPDU.length == 5) { // case 2
                    if (commandAPDU != case2APDU) {
                        System.arraycopy(commandAPDU, 0, case2APDU, 0, 4);
                        commandAPDU = case2APDU;
                    }
                    case2APDU[4] = (byte)(Le & 0xFF);
                } else {
                    if (commandAPDU.length == 5 + Lc) { // case 3
                        byte[] temp = new byte[5 + Lc + 1];
                        System.arraycopy(commandAPDU, 0, temp, 0, 5 + Lc);
                        commandAPDU = temp;
                    }
                    commandAPDU[5 + Lc] = (byte)(Le & 0xFF);
                }
                result_length = 0;
                continue;
            }
            if (result_length >= 2) {
                result_length -= 2; // Delete previous status bytes
            }
            System.arraycopy(resp_buffer.data(), 0, 
                output_buffer.expand(result_length + bytes_read), 
                    result_length, bytes_read);
            result_length += bytes_read;
            
            if (Le == 0 || (sw1 != 0x61 &&
                (channel != 0 || !SIMPresent ||
                 (sw1 != 0x62 && sw1 != 0x63 &&
                  sw1 != 0x9E && sw1 != 0x9F)))) {
                break;
            }

            Le = sw1 == 0x62 || sw1 == 0x63 ? 0 : sw2;
            
            commandAPDU = getResponseAPDU;
            commandAPDU[0] = (byte) channel;
            commandAPDU[4] = (byte)Le;
            if (Le == 0) {
                Le = 256;
            }
        }
        cardSlot.unlockSlot();
        result = new byte[result_length];
        System.arraycopy(output_buffer.data(), 0, result, 0, result_length);
        return result;
    }
    
    /**
     * This method returns the ATR received from the card that this
     * CadClient object is used to communicate with.
     * @return ATR information received from the card at startup or
     * reset. In case of I/O troubles returns null.
     */
    byte[] getATR() {
        byte[] result;
        
        if (!isAlive()) {
            result = null;
        } else {
            result = cardSlot.getATR();
        }
        if (result == null) {
            SIMPresent = false;
        }
        return result;
    }

    /**
     * This method returns the FCI received from the card.
     * @return FCI information received from the card at 
     * selectApplication.
     */
    byte[] getFCI() {
        return FCI;
    }

    /**
     * Returns the card session identifier.
     * This number is different for different card sessions.
     * @return the card session identifier
     */
    public int getCardSessionId() {
        return cardSlot.getCardSessionId();
    }
    
    /**
     * This method is called when there is a connection creation is
     * in progress
     * and specifically card application selection is required.
     * This method also does the channel management part. If basic channel
     * is available for use, this method tries to select the target
     * application
     * on the basic channel. Otherwise, it requests the card for the channel
     * and attempts to select the application on that particular channel.
     *
     * If the selection is
     * successful, this method returns the logical channel reserved for
     * communicating with the selected card application.
     * @param forSAT true if selection is made for SAT connection
     * @param selectAPDU byte encoded selection APDU
     * @exception ConnectionNotFoundException when selection is not successful
     * @exception IOException if no channel is available for communication
     * establishment or there are communication problems.
     * @exception IllegalArgumentException if bad APDU provided.
     * @return response apdu from the select application request
     */
    int selectApplication(boolean forSAT, byte[] selectAPDU) 
            throws IOException {

        int channel;

        // Test if 'POWER UP' is needed or a card was changed
        if (!isAlive()) {
            try {
                if (cardSlot.isSAT()) {
                    throw new ConnectionNotFoundException("SIM not found");
                } else {
                    throw new ConnectionNotFoundException("SmartCard not found");
                }
            } catch (IOException e) {
                throw new ConnectionNotFoundException("SmartCard not found");
            }            
        }

        if (cardSlot.initConnection()) {
            clean();
        }

        if (!forSAT) {
            if (cardSlot.isSAT()) {
                SIMPresent = true;
            } else {
                SIMPresent = false;
            }
            if (basicChannelInUse || SIMPresent) {
                // get another channel for communication.
                byte[] response = exchangeApdu(null, getChannelAPDU);
                if (response.length == 2) {
                    // just got back the status word
                    throw new IOException("No logical channel available");
                }
                // new channel number is in the first byte of response
                channel = response[0];
            }
            else {
                basicChannelInUse = true;
                channel = 0;
            }
        }
        else {
            basicChannelInUse = true;
            channel = 0;
            SIMPresent = true;
        }
        
        selectAPDU[0] = (byte)((selectAPDU[0] & 0xFC) | channel);

        byte[] result = exchangeApdu(null, selectAPDU);

        int sw1 = result[result.length - 2] & 0xFF;
        int sw2 = result[result.length - 1] & 0xFF;
        if ((sw1 << 8) + sw2 != 0x9000) {
            closeChannel(channel);
            throw new ConnectionNotFoundException(
                    "Card application selection failed");
        }
        FCI = result;
        return channel;
    }

    /**
     * This method is used to close connection with the card. If the
     * channel number passed to this method is for basic channel then
     * the basic channel is marked as available and nothing is
     * communicated with the card.
     * If the channel is not the basic channel, a request to close the
     * channel is sent to the card.
     * @param channel channel number
     * @exception IOException if a communication error happens
     */
    void closeChannel(int channel) throws IOException {
        
        if (channel == 0) {
            basicChannelInUse = false;
            return;
        }

        try {
            closeChannelAPDU[3] = (byte) channel;
            exchangeApdu(null, closeChannelAPDU);
        } catch (IOException ioException) {
            throw new IOException("Error closing connection");
        }
    }

    /**
     * Checks if the the connection is still live or not.
     *
     * @return <code>true</code> if the connection is alive
     */
    boolean isAlive() {
        if (cardSlot.isAlive()) {
            return true;
        } else {
            SIMPresent = false;
            return false;
        }
    }
    
    /**
     * Initializes ACL for the slot.
     */
    void initACL() {
        cardSlot.initACL();
    }
    
    /**
     * Frees up the resource that was used by this slot.
     * This method is used only in case of error.
     * IOException is ignored.
     */
    void freeSystemResources() {
        try {
            cardSlot.closeSlot();
        }
        catch (IOException e) {}
        
    }
    
    /**
     * Does cleaning of the object.
     */
    void clean() {
        basicChannelInUse = false;
        SIMPresent = false;
        FCI = null;
    }
    
    /**
     * The TempByteArray class contains temporary byte array for  
     * output buffer during APDUExchange.
     */
    class TempByteArray {
        /**
         * The byte array.
         */
        private byte[] value;
        
        /**
         * The constructor.
         * @param size Initial size.
         */
        TempByteArray(int size) {
            value = new byte[size];
        }
        
        /**
         * Default constructor.
         */
        TempByteArray() {
            value = new byte[256 + 2];
        }
        
        /**
         * Gets value of byte by index.
         * @param i Index.
         * @return Value for i.
         */
        byte val(int i) {
            return value[i];
        }
        
        /**
         * Gets all data as byte array.
         * @return Buffer as byte array.
         */
        byte[] data() {
            return value;
        }
        
        /**
         * Expands the buffer when it too small to hold newsize bytes.
         * If buffer contains data they can be lost.
         * @param newsize Minimum size of buffer.
         * @return Buffer as byte array.
         */
        byte[] ensure(int newsize) {
            if (value.length < newsize) {
                value = new byte[newsize];
            }
            return value;
        }
        
        /**
         * Expands the buffer when it too small to hold newsize bytes.
         * @param newsize Minimum size of buffer.
         * @return Buffer as byte array.
         */
        byte[] expand(int newsize) {
            if (value.length < newsize) {
                byte[] temp = new byte[newsize];
                System.arraycopy(value, 0, temp, 0, value.length);
                value = temp;
            }
            return value;
        }
    }
}
