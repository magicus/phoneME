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
 * The <code>CardRuntimeException</code> class
 * defines a field <code>reason </code>and two accessor methods <code>
 * getReason()</code> and <code>setReason()</code>. The <code>reason</code>
 * field encapsulates an exception cause identifier in Java Card.
 * All Java Card unchecked exception classes should extend
 * <code>CardRuntimeException</code>. 
 */

public class CardRuntimeException extends RuntimeException {
    /** Reason code for the detected error. */
    private short reason;
    
    /**
     * Constructs a <code>CardRuntimeException</code> instance with
     * the specified reason.
     * @param reason the reason for the exception
     */
    public CardRuntimeException(short reason) {
	setReason(reason);
    }
    
    /**
     * Gets the reason code.
     * @return the reason for the exception
     * @see #setReason
     */
    public short getReason() {
	return reason;
    }
    
    /** 
     * Sets the reason code.
     * @param reason the reason for the exception
     * @see #getReason
     */
    public void setReason(short reason) {
	this.reason = reason;
    }
    
    /**
     * Throws an instance of the <code>CardRuntimeException</code>
     * class with the specified reason.
     * @param reason the reason for the exception
     * @exception CardRuntimeException always
     */
    public static void throwIt(short reason) throws CardRuntimeException {  
	throw new CardRuntimeException(reason);
    }
}
