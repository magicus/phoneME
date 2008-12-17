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

package com.sun.midp.automation;

/**
 * Represents MIDlet's lifecycle state.
 */
public final class AutoMIDletLifeCycleState {
    /** External state name */
    private String name;

    /**
     * State representing following condition:
     * MIDlet is paused.
     */
    public static final AutoMIDletLifeCycleState PAUSED = 
        new AutoMIDletLifeCycleState("paused");

    /**
     * State representing following condition:
     * MIDlet is active.
     */
    public static final AutoMIDletLifeCycleState ACTIVE = 
        new AutoMIDletLifeCycleState("active");
    
    /**
     * State representing following condition:
     * MIDlet is destroyed.
     */    
    public static final AutoMIDletLifeCycleState DESTROYED = 
        new AutoMIDletLifeCycleState("destroyed");

    /**
     * Returns a string representation of the state.
     *
     * @return string representation of the state
     */
    public String toString() {
        return name;
    }

    /**
     * Private constructor to prevent other states creation
     */
    private AutoMIDletLifeCycleState() {
    }

    /**
     * Constructor.
     *
     * @param name external state name
     */
    private AutoMIDletLifeCycleState(String name) {
        this.name = name;
    }    
}
