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

package java.rmi;

/**
 * A <code>RemoteException</code> is the common superclass for a number of
 * communication-related exceptions that may occur during the execution of a
 * remote method call.  Each method of a remote interface, an interface that
 * extends <code>java.rmi.Remote</code>, must list
 * <code>RemoteException</code> in its throws clause.
 *
 * @version 1.18, 10/27/00
 * @since   JDK1.1
 */
public class RemoteException extends java.io.IOException {

    /* indicate compatibility with JDK 1.1.x version of class */
    private static final long serialVersionUID = -5148567311918794206L;

    /**
     * Nested Exception to hold wrapped remote exception.
     *
     * <p>This field predates the general-purpose exception chaining facility.
     *
     * @serial
     * @since   JDK1.1
     */
    public Throwable detail;

    /**
     * Constructs a <code>RemoteException</code> with no specified
     * detail message.
     * @since   JDK1.1
     */
    public RemoteException() {}

    /**
     * Constructs a <code>RemoteException</code> with the specified
     * detail message.
     *
     * @param s the detail message
     * @since   JDK1.1
     */
    public RemoteException(String s) {
	super(s);
    }

    /**
     * Constructs a <code>RemoteException</code> with the specified
     * detail message and nested exception.
     *
     * @param s the detail message
     * @param ex the nested exception
     * @since   JDK1.1
     */
    public RemoteException(String s, Throwable ex) {
	super(s);
	detail = ex;
    }

    /**
     * Returns the detail message, including the message from the nested
     * exception if there is one.
     * @since   JDK1.1
     */
    public String getMessage() {
	if (detail == null)
	    return super.getMessage();
	else
	    return super.getMessage() +
		"; nested exception is: \n\t" +
		detail.toString();
    }
}
