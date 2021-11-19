/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

import com.sun.j2me.app.AppPackage;
import com.sun.j2me.security.SatsaPermission;
import javax.microedition.io.*;
import javax.microedition.apdu.*;
import com.sun.j2me.io.ConnectionBaseInterface;
import com.sun.satsa.acl.ACLPermissions;
import com.sun.satsa.acl.AccessControlManager;
import com.sun.satsa.acl.APDUPermissions;
import com.sun.satsa.util.Utils;
import com.sun.satsa.util.Constants;

import java.io.*;

import com.sun.j2me.security.TrustedClass;
import com.sun.j2me.security.Token;
import com.sun.satsa.security.SecurityInitializer;

/**
 * This is the implementation class for APDUConnection interface and provides
 * a high-level API to the J2ME applications allowing them to connect and
 * communicate with the card applications. An instance of this class is
 * created when Connector.open method is called with 'apdu' as protocol. An
 * instance of this class is only returned to the calling J2ME application if
 * the card application selection is successful. If there are any errors that
 * occur during the card application selection, IOException is thrown.
 * The application calls <tt>Connector.open</tt> with an APDU URL string and
 * obtains a {@link javax.microedition.apdu.APDUConnection} object.
 *
 */
public class Protocol implements APDUConnection, ConnectionBaseInterface,
                                 StreamConnection {

    /*
     * Inner class to request security token from SecurityTokenInitializer.
     * SecurityTokenInitializer should be able to check this inner class name.
     */
    static private class SecurityTrusted
        implements TrustedClass { };

    /** This class has a different security domain than the App suite */
    private static Token securityToken =
        SecurityInitializer.requestToken(new SecurityTrusted());

    /**
     * This object verifies access rights of the MIDlet.
     */
    private APDUPermissions verifier;

    /**
     * This flag is to indicate if this APDU connection is to be used for
     * communicating with SAT.
     */
    private boolean openForSAT;

    /**
     * Connection handle.
     */
    private Handle h;

    /**
     * Opens a connection.
     *
     * @param name the target of the connection
     * @param mode indicates whether the caller
     *             intends to write to the connection. Currently,
     *             this parameter is ignored.
     * @param timeouts indicates whether the caller
     *                 wants timeout exceptions. Currently,
     *             this parameter is ignored.
     * @return this connection
     * @throws IOException if the connection is closed or unavailable
     * @throws SecurityException if access is restricted by ACL
     */
    public Connection openPrim(String name, int mode, boolean timeouts)
            throws IOException {

        // parse the URI for slot number and target
        int slotIndex = name.indexOf(":");
        int targetIndex = name.indexOf(";target=");

        if (targetIndex < 0) {
            throw new IllegalArgumentException(
                    "Target missing in connection URL");
        }

        int slot;

        if (targetIndex == slotIndex + 1) {
            slot = 0;
        } else {
            try {
                slot = Integer.parseInt(
                        name.substring(slotIndex + 1, targetIndex), 16);
            } catch (NumberFormatException e) {
                throw new IllegalArgumentException("Invalid slot number");
            }
        }

        String target = name.substring(targetIndex + 8);

        boolean isSAT = target.equals("SAT");
        
        AppPackage appPackage = AppPackage.getInstance();
        try {
            if (isSAT) {
                appPackage.checkForPermission(
                        SatsaPermission.APDU_CHANNEL0_SAT_OPEN);
            } else {
                appPackage.checkForPermission(
                        SatsaPermission.APDU_CONNECTION_OPEN);
            }
        } catch (InterruptedException ie) {
            throw new InterruptedIOException(
                    "Interrupted while trying to ask the user permission");
        } 

        // open connection

        if (isSAT) {
            boolean satSlot;
            try {
                APDUManager.initACL(slot);
                satSlot = APDUManager.isSatSlot(slot);
            } catch (IllegalArgumentException e) {
                satSlot = false;
            }
            if (!satSlot) {
                throw new ConnectionNotFoundException("Invalid slot for SIM");
            }
            h = APDUManager.openSATConnection(slot);
            openForSAT = true;
        } else {

            APDUManager.checkSlotNumber(slot);

            byte[] apdu = new byte[32];
            apdu[1] = (byte) 0xa4;
            apdu[2] = 4;
            apdu[3] = Constants.P2;

            boolean ok;
            try {
                int len = APDUManager.parseDottedBytes(target, apdu, 5);
                apdu[4] = (byte) len;
                apdu[5 + len] = 127;
                ok = len > 4 && len < 17;
            } catch (NullPointerException npe) {
                ok = false;
            } catch (IndexOutOfBoundsException iobe) {
                ok = false;
            } catch (NumberFormatException nfe) {
                ok = false;
            } catch (IllegalArgumentException iae) {
                ok = false;
            }

            if (! ok) {
                throw new IllegalArgumentException("Invalid AID");
            }

            APDUManager.initACL(slot);
            verifier = AccessControlManager
                .getAPDUPermissions(slot,
                apdu,
                appPackage.getCA());

            h = APDUManager.selectApplication(apdu, slot);
        }
        return this;
    }

    /**
     * Closes the connection.
     * @exception IOException  if an I/O error occurs
     */
    public void close() throws IOException {
        /* 
         * IMPL_NOTE: To pass JDTS test 
         * com.sun.satsa.apdu.exchange.interruptedIOClosed next lines 
         * should be uncommented
         */
         // try {Thread.sleep(1000); } 
         // catch (InterruptedException ignored) {}
         
        if (h != null) {
            Handle w = h;
            h = null;
            APDUManager.closeConnection(w);
        }
    }

    /**
     * Exchanges an APDU command with a smart card application.
     * Communication to a smart card device is synchronous.
     * This method will block until the response has been received
     * from the smart card application, or is interrupted.
     * The interruption could be due to the card being removed from
     * the card access device, the operation may timeout, or the
     * connection may be closed from another thread accessing this
     * connection.
     *
     * @param commandAPDU a byte encoded command for the smart card
     * application
     * @return a byte encoded response to the requested operation
     * @exception IOException is thrown if the operation was not
     * successful, or if the connection was already closed
     * @throws InterruptedIOException if a timeout occurs while
     * either trying to send the command or if this <code>Connection</code>
     * object is closed during this exchange operation
     * @throws NullPointerException if the parameter is null
     * @throws SecurityException if the application does not
     *         have permission to exchange the message
     */
    public byte[] exchangeAPDU(byte[] commandAPDU) throws
        IOException, InterruptedIOException {
            
        checkHandle(h);
        if (commandAPDU == null || commandAPDU.length < 4) {
            throw new IllegalArgumentException();
        }

        int nibble = commandAPDU[0] & 0xF0;
        boolean channelEncoded = (nibble == 0 ||
                                  (nibble >= 0x80 && nibble <= 0xA0));

        if (channelEncoded) {
            // mask off the channel information
            commandAPDU[0] &= 0xFC;
        }

        int command = Utils.getInt(commandAPDU, 0);

        if (openForSAT) {
            // check if this is an envelope by checking the INS, P1 and
            // P2 which should have values 0xC2, 0 and 0 respectively.
            if ((command & 0xffffff) != 0xC20000) {
                throw new IllegalArgumentException("Non-envelope APDU");
            }
            commandAPDU[0] = (byte) 0x80;
        } else {

            int cmd = command >> 8;
            if (cmd == 0xA404 || cmd == 0x7000 || cmd == 0x7080) {
                throw new IllegalArgumentException
                    ("Selection or channel management APDUs are not allowed");
            }

            // if channel is non-zero, valid CLA bytes are only:
            // 0x0X, 0x8X, 0x9X or 0xAX i.e. the one that can have
            // the channel information encoded in it
            if (h.channel != 0 && ! channelEncoded) {
                throw new IllegalArgumentException
                        ("Invalid CLA byte for a non-zero channel");
            }

            // check if allowed by the ACL
            verifier.checkPermission(command);

            if (channelEncoded) {
                // set channel bits in two LSB
                commandAPDU[0] |= h.channel;
            }
        }
        return APDUManager.exchangeAPDU(h, commandAPDU);
    }

    /**
     * Returns the ATR message sent by smart card in response to the
     * reset operation.
     * @return the ATR response message, or <code>null</code>
     * if there is no message available
     */
    public byte[] getATR() {
        byte[] result = null;
        try {
            checkHandle(h);
            result = h.getATR();
        } catch (IOException e) {
            result = null;
        }
        return result;
    }

    /**
     * This method always throw <code>IllegalArgumentException</code>.
     * @return An input stream
     * @exception IOException  If an I/O error occurs
     * @exception IllegalArgumentException  is thrown for all requests
     */
    public InputStream openInputStream() throws IOException {
        throw new IllegalArgumentException("Not supported");
    }

    /**
     * This method always throw <code>IllegalArgumentException</code>.
     * @return                 An input stream
     * @exception IOException  If an I/O error occurs
     * @exception IllegalArgumentException  is thrown for all requests
     */
    public DataInputStream openDataInputStream() throws IOException {
        throw new IllegalArgumentException("Not supported");
    }

    /**
     * This method always throw
     * <code>IllegalArgumentException</code>.
     * @return                 An output stream
     * @exception IOException  If an I/O error occurs
     * @exception IllegalArgumentException  is thrown for all requests
     */
    public OutputStream openOutputStream() throws IOException {
        throw new IllegalArgumentException("Not supported");
    }

    /**
     * This method always throw
     * <code>IllegalArgumentException</code>.
     *
     * @return                 An output stream
     * @exception IOException  If an I/O error occurs
     * @exception IllegalArgumentException  is thrown for all requests
     */
    public DataOutputStream openDataOutputStream() throws IOException {
        throw new IllegalArgumentException("Not supported");
    }

    /**
     * A call to enterPin method pops up a UI that requests the PIN
     * from the user. The pinID field indicates which PIN must be
     * requested from the user. The user can
     * either cancel the request
     * or continue. If the user enters the PIN and chooses to continue the
     * implementation is responsible
     * for presenting the PIN value to the card for verification.
     * @param pinID the type of PIN the implementation is suppose to prompt
     * the user to enter.
     * @return result of PIN verification which is the status word
     * recived from the smart card in the form of a byte array. This method
     * would return null if the user cancels the request.
     * @exception IOException is thrown if the PIN could not be communicated
     * with the card due to IO problems such as if the connection was
     * closed before the command could be completed successfully.
     * @exception InterruptedIOException is thrown if the connection object
     * is closed before a reply from the card is received.
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for PIN verification.
     */
    public byte[] enterPin(int pinID) throws IOException {
        return doEnterPin(pinID, 0, ACLPermissions.CMD_VERIFY);
    }

    /**
     * A call to <code>changePin</code> method pops up a UI that requests the
     * the user for an old or existing PIN value and the new PIN value
     * to change the value of the PIN. The pinID field indicates which PIN is
     * to be changed. The user can
     * either cancel the request
     * or continue. If the user enters the PIN values and chooses to
     * continue the
     * implementation is responsible
     * for presenting the PIN value to the card to the card.
     * @param pinID the type of PIN the implementation is suppose to prompt
     * the user to change.
     * @return result of changing the PIN value which is the status word
     * recived from the smart card in the form of a byte array. This method
     * would return null if the user cancels the request.
     * @exception IOException is thrown if the PIN could not be communicated
     * with the card due to IO problems such as if the connection was
     * closed before the command could be completed successfully.
     * @exception InterruptedIOException is thrown if the connection object
     * is closed before a reply from the card is received.
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for changing the PIN value.
     */
    public byte [] changePin(int pinID) throws IOException {
        return doEnterPin(pinID, 0, ACLPermissions.CMD_CHANGE);
    }

    /**
     * A call to <code>disablePin</code> method pops up a UI that requests the
     * the user to enter the value for the PIN that is to be disabled.
     * The pinID field
     * indicates which PIN is to be disabled. The user can
     * either cancel the request
     * or continue. If the user enters the PIN and chooses to continue the
     * implementation is responsible
     * for presenting the PIN value to the card to disable PIN.
     * @param pinID the type of PIN the implementation is required to prompt
     * the user to enter.
     * @return result of disabling the PIN value which is the status word
     * recived from the smart card in the form of a byte array. This method
     * would return null if the user cancels the request.
     * @exception IOException is thrown if the PIN could not be communicated
     * with the card due to IO problems such as if the connection was
     * closed before the command could be completed successfully.
     * @exception InterruptedIOException is thrown if the connection object
     * is closed before a reply from the card is received.
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for disabling the PIN.
     */
    public byte [] disablePin(int pinID) throws IOException {
        return doEnterPin(pinID, 0, ACLPermissions.CMD_DISABLE);
    }

    /**
     * A call to <code>enablePin</code> method pops up a UI that requests the
     * the user to enter the value for the PIN that is to be enabled.
     * The pinID field
     * indicates which PIN is to be enabled. The user can
     * either cancel the request
     * or continue. If the user enters the PIN and chooses to continue the
     * implementation is responsible
     * for presenting the PIN value to the card for enabling the PIN.
     * @param pinID the type of PIN the implementation is required to prompt
     * the user to enter.
     * @return result of enabling the PIN value which is the status word
     * recived from the smart card in the form of a byte array. This method
     * would return null if the user cancels the request.
     * @exception IOException is thrown if the PIN could not be communicated
     * with the card due to IO problems such as if the connection was
     * closed before the command could be completed successfully.
     * @exception InterruptedIOException is thrown if the connection object
     * is closed before a reply from the card is received.
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for enabling the PIN.
     */
    public byte [] enablePin(int pinID) throws IOException {
        return doEnterPin(pinID, 0, ACLPermissions.CMD_ENABLE);
    }

    /**
     * This is a high-level method that lets the J2ME application
     * ask the user to enter the value for an unblocking PIN,
     * and the new value for the blocked PIN and send
     * these to the card.
     * A call to <code>unblockPin</code> method pops up a UI that requests
     * the user to enter the value for the unblocking PIN and the
     * new value for the blocked PIN.
     * The <code>unblockingPinID</code> field indicates which unblocking
     * PIN is to be
     * used to unblock the blocked PIN which is indicated by the field
     * <code>blockedPinId</code>.
     * The unblockingPinID field indicates which PIN is to be unblocked.
     * The user can either cancel the request
     * or continue. If the user enters the PIN values and chooses to continue,
     * the implementation is responsible
     * for presenting the PIN values to the card for unblocking the
     * blocked PIN.
     * If padding is required for either of the PIN values, the
     * implementation is responsible for providing appropriate padding.
     * @param blockedPinID the Id of PIN that is to be unblocked.
     * @param unblockingPinId the Id of unblocking PIN.
     * @return result of unblocking the PIN value which is the status word
     * received from the smart card in the form of a byte array. This method
     * would return null if the user cancels the request.
     * @exception IOException is thrown if the PIN could not be communicated
     * with the card because the connection was
     * closed before this method was called or because
     * of communication problems.
     * @throws InterruptedIOException can be thrown in any of these situations:
     * <ul>
     *    <li>if this <code>Connection</code>
     *        object is closed during the exchange
     *        operation</li>
     *     <li>if the card is removed after connection is established and
     *         then reinserted, and attempt is made to unblock PIN
     *         without re-establishing the connection</li>
     * </ul>
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for unblocking the PIN.
     */
    public byte [] unblockPin(int blockedPinID, int unblockingPinId)
    throws IOException {
        return doEnterPin(blockedPinID, unblockingPinId,
                          ACLPermissions.CMD_UNBLOCK);
    }

    /**
     * Performs PIN entry operation.
     * @param pinID PIN identifier.
     * @param uPinID unblocking PIN identifier.
     * @param action PIN operation identifier.
     * @return result of PIN verification which is the status word
     * received from the smart card in the form of a byte array. This method
     * would return null if the user cancels the request.
     * @exception IOException is thrown if the PIN could not be communicated
     * with the card due to IO problems such as if the connection was
     * closed before the command could be completed successfully.
     * @exception InterruptedIOException is thrown if the connection object
     * is closed before a reply from the card is received.
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for PIN verification.
     */
    private byte[] doEnterPin(int pinID, int uPinID, int action)
            throws IOException {

        checkHandle(h);

        if (openForSAT) {
            throw new SecurityException();
        }

        int header = verifier.preparePIN(pinID, uPinID, action);

        Object[] pins = verifier.enterPIN(action, securityToken);

        if (pins == null) {
            return null;
        }

        byte[] pin1 = (byte[]) pins[0];
        byte[] pin2 = (pins.length == 2) ? (byte[]) pins[1] : null;

        int dataSize = pin1.length;
        if (pin2 != null) {
            dataSize += pin2.length;
        }

        byte[] command = new byte[6 + dataSize];

        command[0] = (byte) (header >> 24);
        command[1] = (byte) (header >> 16);
        command[2] = (byte) (header >> 8);
        command[3] = (byte) header;
        command[4] = (byte) dataSize;
        System.arraycopy(pin1, 0, command, 5, pin1.length);

        if (pin2 != null) {
            System.arraycopy(pin2, 0, command, 5 + pin1.length, pin2.length);
        }
        command[5 + dataSize] = 0x7f;

        byte[] result;
        try {
            // CLA can be incompatible with this logical channel
            result = exchangeAPDU(command);
        } catch (IllegalArgumentException e) {
            throw new IOException(e.getMessage());
        }
        byte[] out = new byte[2];
        System.arraycopy(result, result.length - 2, out, 0, 2);
        return out;
    }
    
    /**
     * Checks if given handle is still valid.
     * @param h Handle to be checked.
     * @exception IOException is thrown if connection is closed
     * @exception InterruptedIOException is thrown if connection 
     * closed and re-established again.
     */
    private void checkHandle(Handle h) throws IOException {
        if (h == null || !h.opened 
                || h.getCardSessionId() == -1) {
            throw new IOException("Connection closed");
        }
        if (h.getCardSessionId() != h.cardSessionId) {
            throw new InterruptedIOException("Connection closed");
        }
    }
}
