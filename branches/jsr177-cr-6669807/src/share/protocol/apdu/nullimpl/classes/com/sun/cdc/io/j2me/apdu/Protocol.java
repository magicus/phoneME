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

import javax.microedition.io.Connection;
import javax.microedition.io.ConnectionNotFoundException;
import javax.microedition.apdu.APDUConnection;
import com.sun.j2me.io.ConnectionBaseInterface;

import java.io.IOException;
import java.io.InterruptedIOException;

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
public class Protocol implements APDUConnection, ConnectionBaseInterface {

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
		throw new ConnectionNotFoundException("Null implementation: " +
			"J2ME device does not have the smart card slot");
    }

	    /**
     * Closes the connection.
     * @exception IOException  if an I/O error occurs
     */
    public void close() throws IOException {
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
	    return null;
    }

    /**
     * Returns the ATR message sent by smart card in response to the
     * reset operation.
     * @return the ATR response message, or <code>null</code>
     * if there is no message available
     */
    public byte[] getATR() {
		return null;
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
        return null;
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
		return null;
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
		return null;
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
		return null;
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
		return null;
    }
}
