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

package javax.microedition.jcrmi;

import javax.microedition.io.*;

// JAVADOC COMMENT ELIDED

public interface JavaCardRMIConnection extends Connection {
    /**
     * This status is returned to the calling J2ME application
     * if the operation for PIN verification/change/disable/
     * enable/unblock was not successful because the user
     * cancelled the PIN entry request.
     */
    public static final short PINENTRY_CANCELLED = -1;

    /**
     * Returns the stub object for an initial remote reference.
     * @return the initial remote reference
     */
    java.rmi.Remote getInitialReference();

    // JAVADOC COMMENT ELIDED
    short enterPin(int pinID) throws java.rmi.RemoteException;

    // JAVADOC COMMENT ELIDED
    short changePin(int pinID) throws java.rmi.RemoteException;
    
    // JAVADOC COMMENT ELIDED
    short disablePin(int pinID) throws java.rmi.RemoteException;
    
    // JAVADOC COMMENT ELIDED    
    short enablePin(int pinID) throws java.rmi.RemoteException;
    
    // JAVADOC COMMENT ELIDED
    short unblockPin(int blockedPinID, int unblockingPinID) 
    throws java.rmi.RemoteException;
    
}
