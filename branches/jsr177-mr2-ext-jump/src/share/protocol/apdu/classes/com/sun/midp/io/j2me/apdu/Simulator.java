/*
 *   
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package  com.sun.midp.io.j2me.apdu;

import com.sun.cardreader.*;
import com.sun.j2me.io.ConnectionBaseAdapter;
import com.sun.j2me.io.PrivilegedConnector;
import java.io.IOException;
import java.util.*;
import java.io.*;
import javax.microedition.io.Connection;
import com.sun.j2me.main.Configuration;

/**
 * This class provides virtual device driving the simulator.
 */
public class Simulator extends com.sun.cardreader.CardDevice {
    /** Configuration property name. */
    public static final String PROP_NAME = 
        "com.sun.midp.io.j2me.apdu.hostsandports";
    /** Host names. */
    private static String[] hosts = new String[0];

    /** Port numbers. */
    private static int[] ports = new int[0];

    /** Input streams. */
    private InputStream[] in = new InputStream[0];

    /** Output streams. */
    private OutputStream[] out = new OutputStream[0];

    /**
     * Socket connection to connect with the Java Card reference
     * implementation.
     */
    private Connection[] conn = 
                new Connection[0];

    /**
     * Number of slots supported by the simulator.
     */
    private int slotCount = 0;

    /**
     * Number of the current slot for operations.
     */
    private int currentSlot = 0;

    /**
     * Is the card changed since last reset.
     */
    private boolean cardChanged;

    /** The response message received from the card. */
    private TLP224Message responseMsg;

    /** Message sent to the card. */
    private TLP224Message commandMsg;

    /** Byte array containing current APDU header. */
    private byte[] header;

    /** Current command APDU data. */
    private byte[] command;

    /** Lc value for the current APDU (data size). */
    private int Lc;

    /** Le value for the current APDU (length of expected data). */
    private int Le;

    /** 1st byte of APDU status word. */
    private int sw1;

    /** 2nd byte of APDU status word. */
    private int sw2;

    /** Data received from the card during APDU exchange. */
    private byte[] output;

    /** Current size of received data. */
    private int dataSize;

    /**
     * Initializes the device.
     * 
     * @throws IOException If device initialization failed.
     * @throws CardDeviceException If device configuration is bad.
     */
    public void init() throws IOException, CardDeviceException {

        /* Read configuration, moved from Cad */
        String config = Configuration.getProperty(PROP_NAME);

        boolean ok;
        try {
            Vector list = new Vector();

            int index = config.indexOf(",");
            while (index != -1) {
                list.addElement(config.substring(0, index));
                config = config.substring(index + 1);
                index = config.indexOf(",");
            }
            list.addElement(config);

            this.slotCount = list.size();
            this.hosts = new String[this.slotCount];
            this.ports = new int[this.slotCount];
            this.in = new InputStream[this.slotCount];
            this.out = new OutputStream[this.slotCount];
            this.conn = 
                new Connection[this.slotCount];

            for (int i = 0; i < list.size(); i++) {
                String s = (String) list.elementAt(i);
                index = s.indexOf(":");
                hosts[i] = s.substring(0, index++).trim();
                ports[i] = Integer.parseInt(s.substring(index));
            }
            ok = list.size() > 0;
        } catch (NullPointerException npe) {
            ok = false;
        } catch (IndexOutOfBoundsException iobe) {
            ok = false;
        } catch (NumberFormatException nfe) {
            ok = false;
        }

        if (! ok) {
            throw new CardDeviceException("Simulator configuration is bad");
        }
        header = new byte[4];
        commandMsg = new TLP224Message();
        responseMsg = new TLP224Message();
        cardChanged = false;
    }

    /**
     * Close the specified slot. 
     * @param slot Slot number
     * @throws IOException If slot close failed.
     */
    public void closeSlot(int slot) throws IOException {
        String msg = "";
        if (slot >= 0 && slot <= this.slotCount) {
            
            try {
                if (this.in[slot] != null) {
                    InputStream tmp = this.in[slot];
                    this.in[slot] = null;
                    tmp.close();
                }
            } catch (IOException e) {
                msg += e.getMessage();
            }
            
            try {
                if (this.out[slot] != null) {
                    OutputStream tmp = this.out[slot];
                    this.out[slot] = null;
                    tmp.close();
                }
            } catch (IOException e) {
                msg += e.getMessage();
            }
            
            try {
                if (this.conn[slot] != null) {
                    Connection tmp = this.conn[slot];
                    this.conn[slot] = null;
                    tmp.close();
                }
            } catch (IOException e) {
                msg += e.getMessage();
            }
            if (msg.length() != 0) {
                throw new IOException(msg);
            }
        } else {
            throw new IOException("Invalid slot number");
        }
    }
    

    /**
     * Closes the device.
     * No exceptions thrown to avoid mess in exception handlers trying
     * to clean up and close the device.
     */
    public void close() {
        for (int i = 0; i < this.slotCount; i++) {
            try {
                closeSlot(i);
            }
            catch (IOException e) { // ignored
            }
        }
    }
    
    /**
     * Performs platform lock of the device. This is intended to make
     * sure that no other native application
     * uses the same device during a transaction.
     *
     * @throws IOException If a device locking failed.
     */
    public void lock() throws IOException {
    }
    
    /**
     * Unlocks the device.
     *
     * @throws IOException If a device unlocking failed.
     */
    public void unlock() throws IOException {
    }
    
    /**
     * Open the specified slot. 
     *
     * @param slot Slot number
     * @param token Security token
     * @throws IOException If slot opening failed.
     */
    public void openSlot(int slot) throws IOException {
        // IMPL_NOTE: The CREF starts very slow. TCK's port checking is not enough:
        // the CREF can accept connection a little later than port 
        // is not available. Two seconds is enough on my computer
        try {
            Thread.sleep(2000);
        } catch (InterruptedException ie) {} // ignored

        // Here we open socket to the simulator
        String url = "//" + this.hosts[slot] + ":" + this.ports[slot];
        try {
            this.conn[slot] = PrivilegedConnector.openPrim(url);
            this.in[slot] = ((ConnectionBaseAdapter)this.
                                    conn[slot]).openInputStream();
            this.out[slot] = ((ConnectionBaseAdapter)this.
                                    conn[slot]).openOutputStream();
        } catch (IOException ie) {
            throw new IOException("Cannot open '" + url + "':" + ie);
        }
    }

    /**
     * Selects the current slot for the subsequent transfer operations.
     * For the one-slot devices the default slot number is 0.
     *
     * @param slot Slot number
     * @throws IOException If a slot selection failed.
     */
    public void selectSlot(int slot) throws IOException {
        if (this.conn[slot] == null) {
            throw new IOException("Connection for simulator slot " +
                                  Integer.toString(slot) + " not open");
        }

        this.currentSlot = slot;
        cardChanged = false;
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
        // IMPL_NOTE: This method must return always false if SAT support is disabled
        return slotNumber == 0;
    }

    /**
     * Performs reset of device.
     * Returns ATR into provided buffer
     *
     * @param atr Buffer for ATR bytes
     * @return Length of ATR
     * @throws IOException If a reset failed.
     */
    public int cmdReset(byte[] atr) throws IOException {
        byte[] data = commandMsg.getData();
        int atrLen;

        data[0] = TLP224Message.ACK;
        data[1] = 4;
        data[2] = TLP224Message.POWER_UP;
        data[3] = 0;
        data[4] = 0;
        data[5] = 0;
        data[6] = commandMsg.computeLRC(6);
        commandMsg.setLength(7);
        sendTLP224Message(commandMsg);
        receiveTLP224Message(responseMsg);
        
        data = responseMsg.getData();
        if (data[2] == TLP224Message.STATUS_CARD_REMOVED) {
            // the card has been just inserted, try again
            sendTLP224Message(commandMsg);
            receiveTLP224Message(responseMsg);
        }
        if (data[2] != 0) {
            throw new IOException("TLP224Error " + (data[2] & 0xff));
        }
        if (data[1] < 5 || (data[1] == 1 && data[2] == 0)) {
            throw new IOException("CREF is not ready");
        }
        // card on-line
        atrLen = data[5];
        if (atrLen < 0 || atrLen > atr.length) {
            throw new IOException("Invalid length of ATR (" + atrLen + ")");
        }
        System.arraycopy(data, 6, atr, 0, atrLen);

        return atrLen;
    }

    /**
     * Performs 'POWER DOWN' command.
     *
     * @throws IOException If a reset failed.
     */
    public void cmdPowerDown() throws IOException {
        byte[] data = commandMsg.getData();
        int atrLen;

        data[0] = TLP224Message.ACK;
        data[1] = 1;
        data[2] = 0x4D; // 'POWER_DOWN'
        data[3] = commandMsg.computeLRC(3);
        commandMsg.setLength(4);
        sendTLP224Message(commandMsg);
        receiveTLP224Message(responseMsg);
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
        boolean retValue = cardChanged;
        cardChanged = false;
        return retValue;
    }
    
    /**
     * Performs data transfer to the device.
     *
     * @param commandAPDU Request bytes
     * @param response Response bytes
     * @return Length of response
     * @throws IOException If a data transfer failed.
     */
    public int cmdXfer(byte[] commandAPDU, byte[] response) 
	throws IOException {

        System.arraycopy(commandAPDU, 0, header, 0, 4);
    	command = commandAPDU;
   	    dataSize = 0;
   	    
    	if (command.length == 4) { // case 1 -> ISO_IN
    	    Lc = 0;
    	    Le = -1;
    	    isoIn();
    	} else
    	if (command.length == 5) { // case 2 -> ISO_OUT
    	    Lc = 0;
    	    Le = command[4] & 0xFF;
    	    isoOut();
	    } else
    	if (command.length == (command[4] & 0xFF) + 5) { // case 3 -> ISO_IN
    	    Lc = (command[4] & 0xFF);
    	    Le = -1;
    	    isoIn();
	    } else
    	if (command.length > (command[4] & 0xFF) + 5) { // case 4 -> ISO_IN
    	    Lc = (command[4] & 0xFF);
    	    Le = (command[Lc + 5] & 0xFF);
    	    isoIn();
	    } else {
    	    throw new IOException("Malformed APDU");
	    }
	    if (dataSize > 0) {
    	    System.arraycopy(output, 0, response, 0, dataSize);
	    }
	    response[dataSize] = (byte)sw1;
	    response[dataSize+1] = (byte)sw2;

	    return dataSize + 2;
    }

    /**
     * Format and send an ISO_IN command to the CAD.
     * @exception InterruptedIOException if connection was closed in the
     * other thread
     * @exception IOException if a communication error happens
     */
    private void isoIn() throws IOException {

        byte[] data = commandMsg.getData();
        int length = Lc;

        data[0] = TLP224Message.ACK;
        data[1] = (byte) (length + 6);
        data[2] = TLP224Message.ISO_INPUT;
        System.arraycopy(header, 0, data, 3, 4);
        data[7] = (byte) length;
        if (length > 0)
            System.arraycopy(command, 5, data, 8, length);
        data[length + 8] = commandMsg.computeLRC(length + 8);
        commandMsg.setLength(length + 9);

        sendTLP224Message(commandMsg);
        receiveTLP224Message(responseMsg);
        data = responseMsg.getData();
        sw1 = data[3] & 0xff;
        sw2 = data[4] & 0xff;

        byte status = data[2];
        if (status != TLP224Message.STATUS_SUCCESS &&
            status != TLP224Message.STATUS_CARD_ERROR &&
            status != TLP224Message.STATUS_INTERRUPTED_EXCHANGE) {
            throw new IOException("TLP224Error " + status);
        }
    }

    /**
     * Format and send an ISO_OUT command to the CAD.
     * @exception InterruptedIOException if connection was closed in the
     * other thread
     * @exception IOException if a communication error happens
     */
    private void isoOut() throws IOException {

        byte[] data = commandMsg.getData();
        data[0] = TLP224Message.ACK;
        data[1] = 6;
        data[2] = TLP224Message.ISO_OUTPUT;
        System.arraycopy(header, 0, data, 3, 4);
        data[7] = (byte) Le;
        data[8] = commandMsg.computeLRC(8);
        commandMsg.setLength(9);

        sendTLP224Message(commandMsg);
        receiveTLP224Message(responseMsg);

        data = responseMsg.getData();

        int received = (data[1] == 3) ? 0 : responseMsg.getLength() - 6;

        if (received > 0) {
            if (output == null || output.length < received) {
                output = new byte[received];
            }
            System.arraycopy(data, 3, output, dataSize, received);
            dataSize = received;
        }

        sw1 = data[3 + received] & 0xff;
        sw2 = data[4 + received] & 0xff;

        byte status = data[2];
        if (status != TLP224Message.STATUS_SUCCESS &&
            status != TLP224Message.STATUS_CARD_ERROR &&
            status != TLP224Message.STATUS_INTERRUPTED_EXCHANGE) {
            throw new IOException("TLP224Error " + status);
        }
    }

    /**
     * Receive a TLP224 formatted message from the input stream.
     * This method reads bytes from the input stream until an EOT (0x03)
     * character is received. The resulting message is decoded and stored
     * in the TL224Message msg. In the event of a transmission error, this
     * method will send attempt error recovery by sending a TLP224 NACK
     * message to the sender. Up to 5 retries will be performed.
     * @param msg The object to store the received message in.
     * @exception IOException If an error occurs while reading from the input
     * stream.
     */
    private void receiveTLP224Message(TLP224Message msg) throws IOException {

        byte[] data = msg.getData();
        int tries = 0;

        try {
            while (true) {
                // Only retry link level errors 5 times before giving up.
                if (tries++ > 5) {
                    throw new IOException("TLP224Error " + 1);
                }
    
                // loop reading characters until EOT is received.
                boolean messageTooLong = false;
                boolean xmitError = false;
                int got = 0;
    
                int hiNibble, lowNibble;
    
                while ((hiNibble = 
                        in[currentSlot].read()) != TLP224Message.EOT) {
    
                    if (hiNibble == -1) {
                        throw new EOFException();
                    }
    
                    if ((lowNibble = in[currentSlot].read()) == -1) {
                        throw new EOFException();
                    }
    
                    xmitError |= (hiNibble < '0' || hiNibble > 'F' ||
                                  (hiNibble > '9' && hiNibble < 'A')) ||
                                 (lowNibble < '0' || lowNibble > 'F' ||
                                  (lowNibble > '9' && lowNibble < 'A'));
    
                    if (lowNibble == TLP224Message.EOT) {
                        break;
                    }
    
                    if (xmitError)
                        continue;
    
                    hiNibble -= hiNibble > '9' ? 0x37 : 0x30;
                    lowNibble -= lowNibble > '9' ? 0x37 : 0x30;
    
                    try {
                        data[got++] = (byte) ((hiNibble << 4) | lowNibble);
                    } catch (ArrayIndexOutOfBoundsException e) {
                        messageTooLong = true;
                    }
                }
    
                if (xmitError || got < 3) {
                    transmissionError();
                    continue;
                }
    
                if (messageTooLong) {
                    statusResponse(TLP224Message.STATUS_MESSAGE_TOO_LONG);
                    continue;
                }
    
                if (data[got - 1] != msg.computeLRC(got - 1) ||
                    data[1] != (byte) (got - 3)) {
                    // the message must contain a valid LRC, the second byte
                    // of the message is the command length. The total message
                    // length includes the ACK/NACK, the length and the LRC
                    transmissionError();
                    continue;
                }
    
                // The first byte of the message must be either an ACK or a NACK
                if (data[0] != TLP224Message.ACK &&
                    data[0] != TLP224Message.NACK) {
                    statusResponse(TLP224Message.STATUS_PROTOCOL_ERROR);
                    continue;
                }
    
                msg.setLength(got);
                break;
            }
        } catch (IOException e) {
            cardChanged = true;
            throw e;
        }
    }

    /**
     * Formats a TLP224Message into it's ASCII representation and
     * sends the message on the output stream.
     * @param msg TLP224 encoded message to be sent
     * @exception IOException is thrown in case there are any problems.
    */
    private void sendTLP224Message(TLP224Message msg) throws IOException {
        byte[] data = msg.getData();

        try {
            for (int i = 0; i < msg.getLength(); i++) {
                int nibble = data[i] >> 4 & 0xf;
                out[currentSlot].write(nibble + (nibble < 10 ? 0x30 : 0x37));
                nibble = data[i] & 0xf;
                out[currentSlot].write(nibble + (nibble < 10 ? 0x30 : 0x37));
            }
    
            out[currentSlot].write(TLP224Message.EOT);
            out[currentSlot].flush();
        } catch (IOException e) {
            cardChanged = true;
            throw e;
        }
    }

    /**
     * Send a one byte TLP224 status response.
     * @param code the status response
     * @throws IOException if IO error occurs
     */
    private void statusResponse(int code) throws IOException {
        TLP224Message msg = new TLP224Message();
        byte[] data = msg.getData();
        data[0] = TLP224Message.ACK;
        data[1] = 1;
        data[2] = (byte) code;
        data[3] = msg.computeLRC(3);
        msg.setLength(4);
        sendTLP224Message(msg);
    }

    /**
     * Send a TLP224 Transmission Error response.
     * @throws IOException if IO error occurs
     */
    private void transmissionError() throws IOException {
        TLP224Message msg = new TLP224Message();
        byte[] data = msg.getData();
        data[0] = TLP224Message.NACK;
        data[1] = 0;
        data[2] = msg.computeLRC(2);
        msg.setLength(3);
        sendTLP224Message(msg);
    }

    /**
     * Gets number of slots on a device. Default implementation returns 1 which
     * is ok for most devices.
     *
     * @return Number of slots on a device
     */
    public int getSlotCount() {
        return this.slotCount;
    }

    /* 
     * DEBUG: delete till end of file after debugging
     * /
    /** 
     * Formates byte array as hex string.
     * @param buf array to format
     * @param offset start offset in buffer
     * @param length length of array
     * @return result string
     * /
    public static String bytesToHex(byte[] buf, int offset, int length) {
        StringBuffer sb = new StringBuffer();
        
        for (int i = 0; i < length; i++) {
            int val = (buf[offset + i] & 0xFF);
            if (i != 0) {
                sb.append(' ');
            }
            sb.append(hexDigit(val >> 4));
            sb.append(hexDigit(val & 0x0F));
        }
        return new String(sb);
    }
    
    /**
     * Converts integer value (0-16) into hex representation.
     * @param val value to be converted
     * @return result hex char
     * /
    private static char hexDigit(int val) {
        val &= 0x0F;
        if (val <= 9) {
            return (char)(val + (int)'0');
        }
        return (char)(val - 0x0A + (int)'A');
    }
    
    /**
     * Calculates length of APDU. (For printing purpose.)
     * @param apdu formatted APDU 
     * @return length of APDU command
     * /
    public static int getAPDULength(byte[] apdu) {
        if (apdu == null) {
            return 0;
        }
        if (apdu.length > 5) {
            int Lc = apdu[4] & 0xFF;
            if (apdu.length == Lc + 5) {
                return Lc + 5;
            } else
            if (apdu.length > Lc + 5) {
                return Lc + 6;
            } else {
                return apdu.length;
            }
        } else {
            return apdu.length;
        }
    }
/* */
}
