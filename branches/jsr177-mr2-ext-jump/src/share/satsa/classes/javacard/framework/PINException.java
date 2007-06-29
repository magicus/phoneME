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
 * <code>PINException</code> represents a <code>OwnerPIN</code> class
 * access-related exception.
 */

public class PINException extends CardRuntimeException {

    // PINException reason codes
    /**
     * This reason code is used to indicate that one or more input parameters
     * is out of allowed bounds.
     */
    public static final short ILLEGAL_VALUE = 1;
    
    /**
     * Constructs a <code>PINException</code>.
     * To conserve on resources use <code>throwIt()</code>
     * to use the JCRE owned instance of this class.
     * @param reason the reason for the exception
     */
    public PINException(short reason) {
	super(reason);
    }
    
    /**
     * Throws an instance of <code>PINException</code> with the
     * specified reason.
     * @param reason the reason for the exception.
     * @exception PINException always
     */
    public static void throwIt(short reason) {    
	throw new PINException(reason);
    }
}
