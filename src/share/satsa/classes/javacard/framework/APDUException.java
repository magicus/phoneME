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
    // JAVADOC COMMENT ELIDED
    public static final short ILLEGAL_USE = 1;
    
    // JAVADOC COMMENT ELIDED
    public static final short BUFFER_BOUNDS = 2;
    
    // JAVADOC COMMENT ELIDED
    public static final short BAD_LENGTH = 3;
    
    // JAVADOC COMMENT ELIDED
    public static final short IO_ERROR = 4;
    
    // JAVADOC COMMENT ELIDED
    public static final short NO_T0_GETRESPONSE = 0xAA;
    
    // JAVADOC COMMENT ELIDED
    public static final short T1_IFD_ABORT = 0xAB;
    
    // JAVADOC COMMENT ELIDED
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
