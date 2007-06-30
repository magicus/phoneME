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

package javacard.framework;

/**
 * <code>APDUException</code> represents an <code>APDU</code>-related exception.
 */

public class APDUException extends CardRuntimeException {

  // APDUException reason code 
    /**
     * This <code>APDUException</code> reason code indicates that the
     * method should
     * not be invoked
     * based on the current state of the APDU. 
     */
    public static final short ILLEGAL_USE = 1;
    
    /**
     * This reason code is used by the <code>APDU.sendBytes()</code>
     * method to indicate
     * that the sum of buffer offset parameter and the byte length
     * parameter exceeds the APDU
     * buffer size.
     */
    public static final short BUFFER_BOUNDS = 2;
    
    /**
     * This reason code is used by the
     * <code>APDU.setOutgoingLength()</code> method to indicate
     * that the length parameter is greater that 256 or
     * if non BLOCK CHAINED data transfer is requested and
     * <code>len</code> is greater than
     * (IFSD-2), where IFSD is the Outgoing Block Size. 
     */
    public static final short BAD_LENGTH = 3;
    
    /**
     * This reason code indicates that an unrecoverable error occurred in the
     * I/O transmission layer.
     */
    public static final short IO_ERROR = 4;
    
    /**
     * This reason code indicates that during T=0 protocol, the CAD
     * did not return a GET RESPONSE
     * command in response to a <61xx> response status to send
     * additional data. The outgoing
     * transfer has been aborted. No more data or status can be sent to the CAD 
     * in this <code>APDU.process()</code> method.
     */
    public static final short NO_T0_GETRESPONSE = 0xAA;
    
    /**
     * This reason code indicates that during T=1 protocol, the CAD
     * returned an ABORT S-Block
     * command and aborted the data transfer. The incoming or outgoing
     * transfer has been aborted. No more data can be received from the CAD.
     * No more data or status can be sent to the CAD 
     * in this <code>APDU.process()</code> method.
     */
    public static final short T1_IFD_ABORT = 0xAB;
    
    /**
     * This reason code indicates that during T=0 protocol, the CAD
     * did not reissue the
     * same APDU command with the corrected length in response to a
     * <6Cxx> response status
     * to request command reissue with the specified length. The outgoing
     * transfer has been aborted. No more data or status can be sent to the CAD 
     * in this <code>APDU.process()</code> method.
     */
    public static final short NO_T0_REISSUE = 0xAC;
    
    /**
     * Constructs an APDUException.
     * @param reason the reason for the exception.
     */
    public APDUException(short reason) {
	super(reason);
    }
    
    /**
     * Throws an instance of <code>APDUException</code> with the
     * specified reason.
     * @param reason the reason for the exception.
     * @exception APDUException always.
     */
    public static void throwIt(short reason) {
	throw new APDUException(reason);
    }
}
