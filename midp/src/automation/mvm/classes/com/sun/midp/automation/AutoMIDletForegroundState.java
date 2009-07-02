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
 * Represents MIDlet's foreground state.
 */
public class AutoMIDletForegroundState {
    /** External state name */
    private String name;

    /**
     * Get state representing following condition:
     * MIDlet is in foreground.
     */
    public static AutoMIDletForegroundState FOREGROUND = 
        new AutoMIDletForegroundState("foreground");
    
    /**
     * Get state representing following condition:
     * MIDlet is in background.
     */  
    public static AutoMIDletForegroundState BACKGROUND = 
        new AutoMIDletForegroundState("background");

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
    private AutoMIDletForegroundState() {
    }

    /**
     * Constructor.
     *
     * @param name external state name
     */
    private AutoMIDletForegroundState(String name) {
        name = this.name;
    }
}
