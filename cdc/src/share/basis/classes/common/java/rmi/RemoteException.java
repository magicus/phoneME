/*
 * @(#)RemoteException.java	1.22 03/01/23
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
 * <p>As of release 1.4, this exception has been retrofitted to conform to
 * the general purpose exception-chaining mechanism.  The "wrapped remote
 * exception" that may be provided at construction time and accessed via
 * the public {@link #detail} field is now known as the <i>cause</i>, and
 * may be accessed via the {@link Throwable#getCause()} method, as well as
 * the aforementioned "legacy field."
 *
 * @version 1.22, 01/23/03
 * @author  Ann Wollrath
 * @since   JDK1.1
 */
public class RemoteException extends java.io.IOException {

    /* indicate compatibility with JDK 1.1.x version of class */
    private static final long serialVersionUID = -5148567311918794206L;

    /**
     * Nested Exception to hold wrapped remote exception.
     *
     * <p>This field predates the general-purpose exception chaining facility.
     * The {@link Throwable#getCause()} method is now the preferred means of
     * obtaining this information.
     *
     * @serial
     */
    public Throwable detail;

    /**
     * Constructs a <code>RemoteException</code> with no specified
     * detail message.
     */
    public RemoteException() {
        initCause(null);  // Disallow subsequent initCause
    }

    /**
     * Constructs a <code>RemoteException</code> with the specified
     * detail message.
     *
     * @param s the detail message
     */
    public RemoteException(String s) {
	super(s);
        initCause(null);  // Disallow subsequent initCause
    }

    /**
     * Constructs a <code>RemoteException</code> with the specified
     * detail message and nested exception.
     *
     * @param s the detail message
     * @param ex the nested exception
     */
    public RemoteException(String s, Throwable ex) {
	super(s);
        initCause(null);  // Disallow subsequent initCause
	detail = ex;
    }

    /**
     * Returns the detail message, including the message from the nested
     * exception if there is one.
     * 
     * @return	the detail message, including nested exception message if any
     */
    public String getMessage() {
	if (detail == null) {
	    return super.getMessage();
	} else {
	    return super.getMessage() + "; nested exception is: \n\t" +
		detail.toString();
	}
    }

    /**
     * Returns the wrapped remote exception (the <i>cause</i>).
     *
     * @return  the wrapped remote exception, which may be <tt>null</tt>.
     * @since   1.4
     */
    public Throwable getCause() {
        return detail;
    }
}
