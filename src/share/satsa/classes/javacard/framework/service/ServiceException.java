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

package javacard.framework.service;

import javacard.framework.*;
/**
 * <code>ServiceException</code> represents a service framework
 *  related exception.
 * @version 1.0 
 */
public class ServiceException extends CardRuntimeException {
    
    // ServiceException reason code
    
    /**
     * This reason code is used to indicate that an input parameter is not
     * allowed.
     */
    public static final short ILLEGAL_PARAM = 1;
    
    /**
     * This reason code is used to indicate that a dispatch table is full.
     */
    public static final short DISPATCH_TABLE_FULL = 2;
    
    /**
     * This reason code is used to indicate that the incoming data for a
     * command in the <CODE>APDU</CODE> object does not fit in the APDU buffer.
     */
    public static final short COMMAND_DATA_TOO_LONG = 3;
    
    /**
     * This reason code is used to indicate that the command in the
     * <CODE>APDU</CODE>
     * object cannot be accessed for input processing.
     */
    public static final short CANNOT_ACCESS_IN_COMMAND = 4;
    
    /**
     * This reason code is used to indicate that the command in the
     * <CODE>APDU</CODE> object
     * cannot be accessed for output processing.
     */
    public static final short CANNOT_ACCESS_OUT_COMMAND =  5;
    
    /**
     * This reason code is used to indicate that the command in the
     * <CODE>APDU</CODE> object
     * has been completely processed.
     */
    public static final short COMMAND_IS_FINISHED = 6;
    
    /**
     * This reason code is used by <code>RMIService</code> to indicate
     * that the remote
     * method returned an remote object which has not been exported.
     */
    public static final short REMOTE_OBJECT_NOT_EXPORTED = 7;
    
    /**
     * Constructs a <CODE>ServiceException</CODE>.
     * @param reason the reason for the exception
     */
    public ServiceException(short reason) {
	super(reason);
    }
    
    /**
     * Throws an instance of <code>ServiceException</code> with the
     * specified reason.
     * @param reason the reason for the exception
     * @exception ServiceException always
     */
    public static void throwIt(short reason) throws ServiceException {    
	throw new ServiceException(reason);
    }
    
}

