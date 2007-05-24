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
 * <code>SystemException</code> represents a <code>JCSystem</code>
 * class-related exception.
 */

public class SystemException extends CardRuntimeException {

    // SystemException reason code
    /**
     * This reason code is used to indicate that one or more input parameters
     * is out of allowed bounds.
     */
    public static final short ILLEGAL_VALUE = 1;
    
    /**
     * This reason code is used by the <code>makeTransient*()</code> methods 
     * to indicate that no room is available in volatile memory for
     * the requested object.
     */
    public static final short NO_TRANSIENT_SPACE = 2;
    
    /**
     * This reason code is used to indicate that the request to create
     * a transient object is not allowed in the current applet context.
     * See <em>Java Card Runtime Environment (JCRE)
     * Specification</em>, section 6.2.1 for details.
     */
    public static final short ILLEGAL_TRANSIENT = 3;
    
    /**
     * This reason code is used by the
     * <code>javacard.framework.Applet.register()</code> method
     * to indicate that the input AID parameter is not a legal AID value.
     */
    public static final short ILLEGAL_AID = 4;
    
    /**
     * This reason code is used to indicate that there is insufficient resource
     * in the Card for the request. 
     * <p>For example, the Java Card Virtual Machine may <code>throw</code>
     * this exception reason when there is insufficient heap space to
     * create a new instance.
     */
    public static final short NO_RESOURCE = 5;
    
    /**
     * This reason code is used to indicate that the requested function is not
     * allowed. For example,
     * <CODE>JCSystem.requestObjectDeletion()</CODE> method throws
     * this exception if the object deletion mechanism is not implemented.
     */
    public static final short ILLEGAL_USE = 6;
    
    /**
     * Constructs a SystemException.
     * @param reason the reason for the exception
     */
    
    public SystemException(short reason) {
	super(reason);
    }
    
    /**
     * Throws an instance of <code>SystemException</code> with the
     * specified reason.
     * @param reason the reason for the exception.
     * @exception SystemException always
     */
    public static void throwIt(short reason) throws SystemException {
	throw new SystemException(reason);
    }
}
