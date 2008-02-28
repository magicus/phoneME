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

package javax.microedition.jcrmi;

import java.rmi.RemoteException;

/**
 * The interface <code>RemoteRef</code> represents the handle for
 * a remote object. Each stub contains an instance of
 * <code>RemoteRef</code>. <code>RemoteRef</code> contains the concrete 
 * representation
 * of a reference. This remote reference is used to carry out remote
 * calls on the remote object for which it is a reference.
 *
 */

public interface RemoteRef {

    /**
     * Invokes a remote method.
     * <p>A remote method invocation consists of three steps:</p>
     * <ol>
     * <li>Marshall the representation for the method and parameters.</li>
     * <li>Communicate the method invocation to the host and unmarshall the
     * return value or exception returned.</li>
     * <li>Return the result of the method invocation to the caller.</li>
     * </ol>
     *
     * The remote method invoked on the card can throw an exception to
     * signal that an unexpected condition has been detected.<p>
     *
     * If the exception thrown on the card is an exception defined in
     * the Java Card 2.2 API, then the same exception is thrown to the
     * stub method. The client can access the reason code associated
     * with Java Card-specific exceptions using the standard
     * <code>getReason()</code> method.<p>
     *
     * If the exception thrown on the card is a subclass of an exception
     * defined in the Java Card 2.2 API, then the closest exception defined
     * in the API (along with the reason code, if applicable) is
     * thrown to the stub method. The detail message string of the
     * exception object may indicate that exception subclass was thrown
     * on the card.<p>
     *
     * Apart from the exceptions thrown by the remote method itself,
     * errors during communication, marshalling, protocol handling,
     * unmarshalling, stub object instantiation, and so on, related
     * to the JCRMI method invocation, results in a
     * <code>RemoteException</code> being thrown to the stub method.
     *
     * @param method simple (not fully qualified) name of the method
     *        followed by the method descriptor. Representation of a
     *        method descriptor is the same as that described in The
     *        Java Virtual Machine Specification (section 4.3.3)     
     * @param params the parameter list
     * @return result of remote method invocation
     * @exception java.lang.Exception if any exception occurs during
     *            the remote method invocation
     */

    public Object invoke(String method, Object[] params) throws Exception;

    /**
     * Compares two remote references. Two remote references are equal
     * if they refer to the same remote object.
     * @param obj the Object to compare with
     * @return true if these Objects are equal; false otherwise
     */

    public boolean remoteEquals(RemoteRef obj);

    /**
     * Returns a hashcode for a remote object. Two remote object stubs
     * that refer to the same remote object will have the same hash code.
     *
     * @return the remote object hashcode
     */
    public int remoteHashCode();
}
