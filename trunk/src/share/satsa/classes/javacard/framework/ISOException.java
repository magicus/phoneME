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
 * <code>ISOException</code> class encapsulates an ISO 7816-4 response
 * status word as
 * its <code>reason</code> code.
 */

public class ISOException extends CardRuntimeException {

  
    /**
     * Constructs an ISOException instance with the specified status word.
     * @param sw the ISO 7816-4 defined status word
     */
    public ISOException(short sw) {
	super(sw);
    }
    
    /**
     * Throws an instance of the <code>ISOException</code> class with
     * the specified
     * status word.
     * @param sw ISO 7816-4 defined status word
     * @exception ISOException always
     */
    public static void throwIt(short sw) {
	throw new ISOException(sw);
    }
}
