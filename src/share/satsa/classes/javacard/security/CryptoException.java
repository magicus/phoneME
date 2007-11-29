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

package javacard.security;

import javacard.framework.CardRuntimeException;

/**
 * <code>CryptoException</code> represents a cryptography-related exception.
 */

public class CryptoException extends CardRuntimeException {

  // CryptoException reason codes
    /**
     * This reason code is used to indicate that one or more input parameters
     * is out of allowed bounds.
     */
    public static final short ILLEGAL_VALUE = 1;
    
    /**
     * This reason code is used to indicate that the key is uninitialized.
     */
    public static final short UNINITIALIZED_KEY = 2;
    
    /**
     * This reason code is used to indicate that the requested
     * algorithm or key type
     * is not supported.
     */
    public static final short NO_SUCH_ALGORITHM = 3;
    
    /**
     * This reason code is used to indicate that the signature or
     * cipher object has not been
     * correctly initialized for the requested operation.
     */
    public static final short INVALID_INIT = 4;
    
    /**
     * This reason code is used to indicate that the signature or
     * cipher algorithm does
     * not pad the incoming message and the input message is not block aligned.
     */
    public static final short ILLEGAL_USE = 5;
    
    /**
     * Constructs a <code>CryptoException</code> with the specified reason.
     * To conserve on resources use <code>throwIt()</code>
     * to use the JCRE-owned instance of this class.
     * @param reason the reason for the exception
     */
    public CryptoException(short reason) {
	super(reason);
    }
    
    /**
     * Throws an instance of <code>CryptoException</code> with the
     * specified reason.
     * @param reason the reason for the exception
     * @exception CryptoException always
     */
    public static void throwIt(short reason) {
	throw new CryptoException(reason);
    }
}
