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
 * <code>TransactionException</code> represents an exception in the
 * transaction subsystem.
 */

public class TransactionException extends CardRuntimeException {

    // constants
    /**
     * This reason code is used by the <code>beginTransaction</code>
     * method to indicate
     * a transaction is already in progress.
     */
    // beginTransaction called when already in progress
    public final static short IN_PROGRESS       = 1;    
    
    /**
     * This reason code is used by the <code>abortTransaction</code>
     * and <code>commitTransaction</code> methods
     * when a transaction is not in progress.
     */
    // commit/abortTransaction called when not in progress
    public final static short NOT_IN_PROGRESS   = 2;    
    
    /**
     * This reason code is used during a transaction to indicate that
     * the commit buffer is full.
     */
    // commit buffer is full
    public final static short BUFFER_FULL       = 3;    
    
    /**
     * This reason code is used during a transaction to indicate 
     * an internal JCRE problem (fatal error).
     */
    // internal JCRE problem (fatal error)
    public final static short INTERNAL_FAILURE  = 4;    
    
    /**
     * Constructs a <code>TransactionException</code> with the specified reason.
     * @param reason the internal code indicating the cause of the 
     * error detected
     */
    public TransactionException(short reason) {
	super(reason);
    }
    
    /**
     * Throws an instance of <code>TransactionException</code> with
     * the specified reason.
     * @exception TransactionException always
     * @param reason the internal code indicating the cause of the 
     * error detected
     */
    public static void throwIt(short reason) {
	throw new TransactionException(reason);
    }
}
