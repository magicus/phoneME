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

package com.sun.midp.security;

import java.security.Permission;

public final class RootPermission extends Permission
{

    /**
     * Creates a new RootPermission object with 
     * the specified name.
     * @param name the name of the RootPermission
     */
    public RootPermission(String name)
    {
        super(name);
    }

    /**
     * Check if the specified permission <i>p</i> is 
     * implied by this object.
     * <p>
     * This method returns true if <i>p</i>'s class is
     * the same as this object's class.
     * </p>
     *
     * @param p the permission we are checking against
     * @return true if <i>p</i> is implied
     * by this permission, false otherwise.
     */
    public boolean implies(Permission p)
    {
        return (p instanceof RootPermission);
    }

    /**
     * Checks for Permission object equality.
     *
     * @param o the object we are testing for equality
     * @return true if both objects are equivalent.
     */
    public boolean equals(Object o)
    {
        return (o instanceof RootPermission);
    }

    /**
     * Returns the canonical string representation of the
     * actions, which currently is the empty string, because
     * there are no actions for RootPermission.
     *
     * @return the empty string ""
     */
    public String getActions()
    {
        return "";
    }

    /**
     * Returns the hash code value for this object.
     *
     * @return an hash code value for this object.
     */
    public int hashCode()
    {
        return getName().hashCode();
    }
}
