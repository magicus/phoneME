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

/**
 * The <code>RemoteStub</code> class is the common superclass for stubs of
 * remote objects.<p>
 */

public class RemoteStub {

    /**
     * The remote reference associated with this stub object.
     * The stub uses this reference to perform a remote method call.
     */
    protected RemoteRef ref;

    /**
     * Constructs a <code>RemoteStub</code>.
     */
    public RemoteStub() {}

    /**
     * Sets the remote reference associated with this stub object.
     * @param ref the remote reference
     */
    public void setRef(RemoteRef ref) {

        this.ref = ref;
    }


    /**
     * Compares two remote objects for equality. Two remote objects are
     * equal if their remote references are non-null and equal.
     *
     * @param obj the Object to compare with
     * @return true if these Objects are equal; false otherwise
     */

    public boolean equals(Object obj)  {

        if (obj instanceof RemoteStub) {
            if (ref == null) {
                return obj == this;
            } else {
                return ref.remoteEquals(((RemoteStub)obj).ref);
            }
        }
        return false;
    }

    /**
     * Returns a hashcode for a remote object.  Two remote object stubs
     * that refer to the same remote object will have the same hash code.
     *
     * @return remote object hashcode
     */
    public int hashCode() {
        return (ref == null) ? super.hashCode() : ref.remoteHashCode();
    }
}
