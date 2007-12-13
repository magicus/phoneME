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
 * <code>UserException</code> represents a user exception.
 */

public class UserException extends CardException {

    /**
     * Constructs a <code>UserException</code> with reason = 0.
     */
    public UserException() {
	this((short)0);
    }
    
    /**
     * Constructs a <code>UserException</code> with the specified reason.
     * @param reason the reason for the exception.
     */
    public UserException(short reason) {
	super(reason);
    }
    
    /**
     * Throws an instance of <code>UserException</code> with the
     * specified reason.
     * @param reason the reason for the exception
     * @exception UserException always
     */
    public static void throwIt(short reason) throws UserException {
	throw new UserException(reason);
    }
}
