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

package com.sun.midp.push.controller;

/**
 * Unique identification for <code>MIDlet</code> application.
 *
 * <p>
 * Simple, structure like class.
 * </p>
 */
final class MIDPApp {
    /** <code>MIDlet</code> suite ID. */
    public final int midletSuiteID;

    /** <code>MIDlet</code> class name. */
    public final String midlet;

    /**
     * Constructs an instance.
     *
     * @param midletSuiteID <code>MIDlet suite</code> ID
     * @param midlet <code>MIDlet</code> class name
     */
    public MIDPApp(final int midletSuiteID, final String midlet) {
        this.midletSuiteID = midletSuiteID;
        this.midlet = midlet;
    }

    /** {@inheritDoc} */
    public int hashCode() {
        return (midletSuiteID << 3) + midlet.hashCode();
    }

    /** {@inheritDoc} */
    public boolean equals(final Object obj) {
        if (!(obj instanceof MIDPApp)) {
            return false;
        }

        final MIDPApp rhs = (MIDPApp) obj;
        return (midletSuiteID == rhs.midletSuiteID)
            && midlet.equals(rhs.midlet);
    }
}
