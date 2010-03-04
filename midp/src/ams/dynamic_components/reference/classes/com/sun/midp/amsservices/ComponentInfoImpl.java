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

package com.sun.midp.amsservices;

import com.sun.midp.midlet.MIDletSuite;

/** Simple attribute storage describing a component */
public class ComponentInfoImpl implements ComponentInfo {
    /** ID of this component. */
    public int componentId = ComponentInfo.UNUSED_COMPONENT_ID;
    /** ID of the MIDlet suite which the component belongs to. */
    public int suiteId;
    /** User-friendly name of the component. */
    public String displayName = null;
    /** Version of the component. */
    public String version = null;
    /** Is this component is trusted. */
    public boolean trusted = false;

    /**
     * Constructs an empty ComponentInfo object.
     */
    public ComponentInfoImpl() {
        componentId = ComponentInfo.UNUSED_COMPONENT_ID;
        suiteId = MIDletSuite.UNUSED_SUITE_ID;
        displayName = "";
        version = "";
        trusted = false;
    }

    /**
     * Constructs a ComponentInfo object.
     *
     * @param compId ID of the component
     * @param sId ID of the midlet suite which the component belongs to
     * @param name user-friendly name of the component
     * @param ver version of the component
     * @param isTrusted true if this component is trusted, false otherwise
     */
    public ComponentInfoImpl(int compId, int sId, String name,
                             String ver, boolean isTrusted) {
        componentId = compId;
        suiteId = sId;
        displayName = name;
        version = ver;
        trusted = isTrusted;
    }

    /**
     * Returns ID of this component.
     *
     * @return ID of the component
     */
    public int getComponentId() {
        return componentId;
    }

    /**
     * Returns ID of the midlet suite which this component belongs to.
     *
     * @return ID of the suite which this component belongs to
     */
    public int getSuiteId() {
        return suiteId;
    }

    /**
     * Returns the display name of the component
     * @return user-friendly name of the component
     */
    public String getDisplayName() {
        return displayName;
    }

    /**
     * Returns the version of the component.
     *
     * @return version of the component
     */
    public String getVersion() {
        return version;
    }

    /**
     * Returns true if this component is trusted, false otherwise.
     *
     * @return true if this component is trusted, false otherwise
     */
    public boolean isTrusted() {
        return trusted;
    }

    /**
     * Returns a string representation of the ComponentInfo object.
     * For debug only.
     */
    public String toString() {
        StringBuffer b = new StringBuffer();
        b.append("componentId = " + componentId);
        b.append(", suiteId  = " + suiteId);
        b.append(", displayName = " + displayName);
        b.append(", version = " + version);
        b.append(", trusted = " + trusted);
        return b.toString();
    }
}
