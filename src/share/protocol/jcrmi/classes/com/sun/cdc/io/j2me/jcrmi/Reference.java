/*
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

package com.sun.cdc.io.j2me.jcrmi;

import javax.microedition.jcrmi.RemoteRef;

/** JCRMI remote reference. */
class Reference implements RemoteRef  {

    /**
     * Remote reference uses this connection for card communication.
     */
    private Protocol connection;

    /**
     * Remote object identifier.
     */
    private short objectID;

    /**
     * Anticollision string for remote object.
     */
    private String hashModifier;

    /**
     * The name of the most specific remote interface implemented by remote
     * object.
     */
    private String className;

    /**
     * Creates new remote reference object.
     * @param connection Remote reference uses this connection for
     * card communication.
     * @param objectID Remote object identifier.
     * @param hashModifier Anticollision string for remote object
     * in UTF-8 representation.
     * @param className the name of the most specific remote interface
     * implemented by remote object.
     */
    Reference(Protocol connection,
              short objectID,
              String hashModifier,
              String className) {

        this.connection = connection;
        this.objectID = objectID;
        this.hashModifier = hashModifier;
        this.className = className;
    }

    /**
     * Returns object identifier for this reference.
     * @return object identifier
     */
    short getObjectID() {
        return objectID;
    }

    /**
     * Returns hash modifier for this reference.
     * @return hash modifier
     */
    String getHashModifier() {
        return hashModifier;
    }

    /**
     * Returns the name of the most specific remote interface implemented by
     * remote object.
     * @return the name of interface
     */
    String getClassName() {
        return className;
    }

    /**
     * Invokes a remote method.
     * <p>A remote method invocation consists of three steps:</p>
     * <ol>
     * <li>Marshall the representation for the method and parameters.</p>
     * <p></p>
     * <li>Communicate the method invocation to the host and unmarshall the
     * return value or exception returned.</p>
     * <p></p>
     * <li>Return the result of the method invocation to the caller.</p>
     * <p></p>
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
     *        Java Virtual Machine Specification (&#167 4.3.3)
     * @param params the parameter list
     * @return result of remote method invocation
     * @exception java.lang.Exception if any exception occurs during
     *            the remote method invocation
     */
    public Object invoke(String method, Object[] params)
            throws Exception {
        return connection.invoke(this, method, params);
    }

    /**
     * Compares two remote references. Two remote references are equal
     * if they refer to the same remote object.
     * @param obj - the Object to compare with
     * @return true if these Objects are equal; false otherwise
     */
    public boolean remoteEquals(RemoteRef obj) {

        if (obj == null || ! (obj instanceof Reference)) {
            return false;
        }

        Reference r = (Reference) obj;
        if (r.objectID != objectID) {
            return false;
        }

        if (r.connection == connection) {
            return true;
        }

        return (r.connection.getCardSessionId() ==
                connection.getCardSessionId()) &&
                r.connection.isOpened() &&
                connection.isOpened();
    }

    /**
     * Returns a hashcode for a remote object. Two remote object stubs
     * that refer to the same remote object will have the same hash code.
     *
     * @return the remote object hashcode
     */
    public int remoteHashCode() {
        return connection.getCardSessionId() << 16 + objectID;
    }
}
