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

package com.sun.cdc.io.j2me.jcrmi;

import javax.microedition.io.Connection;
import javax.microedition.io.ConnectionNotFoundException;
import javax.microedition.jcrmi.JavaCardRMIConnection;
import com.sun.j2me.io.ConnectionBaseInterface;

import java.io.IOException;
import java.io.InterruptedIOException;
import java.rmi.RemoteException;

/**
 * JCRMI connection to card application.
 */
public class Protocol
    implements JavaCardRMIConnection, ConnectionBaseInterface {

    /**
     * Connector uses this method to initialize the connection object.
     * This method establishes APDU connection with card application,
     * obtains FCI information and creates stub for initial remote
     * reference.
     * @param name the URL for the connection without protocol name
     * @param mode the access mode (Ignored)
     * @param timeouts a flag to indicate that the caller wants timeout
     *                  exceptions. Ignored
     * @return this connection
     * @throws IOException if the connection can not be initialized
     * @throws RemoteException if initial remote reference object can not be
     * created
     * @throws SecurityException if access is restricted by ACL
     */
    public Connection openPrim(String name, int mode, boolean timeouts)
            throws IOException {
        throw new ConnectionNotFoundException("Null implementation: " +
            "J2ME device does not have the smart card slot");
    }

    /**
     * Closes the connection.
     * @throws IOException If an I/O error occurs
     */
    public void close() throws IOException {
    }

    /**
     * Returns the stub object for an initial remote reference.
     * @return the initial remote reference
     */
    public java.rmi.Remote getInitialReference() {
        return null;
    }

    /**
     * A call to enterPin method pops up a UI that requests the PIN
     * from the user. The pinID field indicates which PIN must be
     * requested from the user. The user can either cancel the request
     * or continue. If the user enters the PIN and chooses to continue,
     * The implementation is responsible for
     * presenting the PIN entered by the user to the card for verification.
     * @param pinID the type of PIN the implementation is suppose to prompt
     * the user to enter.
     * @return PINENTRY_CANCELLED if the user cancelled the PIN entry
     * request or the value returned by the remote method.
     * @exception java.rmi.RemoteException is thrown if the PIN could
     * not be communicated to the card or an exception is thrown
     * by the card in response to the PIN entry.
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for PIN verification.
     */
    public short enterPin(int pinID) throws java.rmi.RemoteException {
        return 0;
    }

    /**
     * A call to <code>changePin</code> method pops up a UI that requests
     * the user for an old or existing PIN value and the new PIN value to
     * to change the value of the PIN. The pinID field indicates which PIN is
     * to be changed. The user can either cancel the request
     * or continue. If the user enters the PIN values and chooses to continue
     * the implementation is responsible for presenting the
     * the old and new values of the PIN to the card.
     * @param pinID the type of PIN the implementation is suppose to prompt
     * the user to change.
     * @return PINENTRY_CANCELLED if the user cancelled the PIN entry
     * request or the value returned by the remote method.
     * @exception java.rmi.RemoteException is thrown if the PIN could
     * not be communicated to the card or an exception is thrown
     * by the card in response to the PIN entry.
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for changing the PIN value.
     */
    public short changePin(int pinID) throws RemoteException {
        return 0;
    }

    /**
     * A call to <code>disablePin</code> method pops up a UI that requests
     * the user to enter the value for the PIN that is to be disabled.
     * The pinID field
     * indicates which PIN is to be disabled. The user can
     * either cancel the request
     * or continue. If the user enters the PIN and chooses to continue the
     * implementation is responsible
     * for presenting the PIN value to the card to disable PIN.
     * @param pinID the type of PIN the implementation is required to prompt
     * the user to enter.
     * @return PINENTRY_CANCELLED if the user cancelled the PIN entry
     * request or the value returned by the remote method.
     * @exception java.rmi.RemoteException is thrown if the PIN could
     * not be communicated to the card or an exception is thrown
     * by the card in response to the PIN entry.
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for disabling the PIN.
     */
    public short disablePin(int pinID) throws RemoteException {
        return 0;
    }

    /**
     * A call to <code>enablePin</code> method pops up a UI that requests
     * the user to enter the value for the PIN that is to be enabled.
     * The pinID field
     * indicates which PIN is to be enabled. The user can
     * either cancel the request
     * or continue. If the user enters the PIN and chooses to continue the
     * implementation is responsible
     * for presenting the PIN value to the card for enabling the PIN.
     * @param pinID the type of PIN the implementation is required to prompt
     * the user to enter.
     * @return PINENTRY_CANCELLED if the user cancelled the PIN entry
     * request or the value returned by the remote method.
     * @exception java.rmi.RemoteException is thrown if the PIN could
     * not be communicated to the card or an exception is thrown
     * by the card in response to the PIN entry.
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for enabling the PIN.
     */
    public short enablePin(int pinID) throws RemoteException {
        return 0;
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
     * @return PINENTRY_CANCELLED if the user cancelled the PIN entry
     * request or the value returned by the remote method.
     * @exception java.rmi.RemoteException is thrown if the PIN could
     * not be communicated to the card or an exception is thrown
     * by the card in response to the PIN entry.
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for unblocking the PIN.
     */
    public short unblockPin(int blockedPinID, int unblockingPinId)
    throws RemoteException {
        return 0;
    }
}
