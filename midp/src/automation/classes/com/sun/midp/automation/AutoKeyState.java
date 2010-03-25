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
import com.sun.midp.lcdui.EventConstants;
import java.util.*;

/**
 * Represents key state.
 */
public final class AutoKeyState {
    /**
     * State: key pressed
     */
    public static final AutoKeyState PRESSED = 
        new AutoKeyState("pressed", EventConstants.PRESSED);

    /**
     * State: key repeated
     */
    public static final AutoKeyState REPEATED = 
        new AutoKeyState("repeated", EventConstants.REPEATED);

    /**
     * State: key released
     */
    public static final AutoKeyState RELEASED = 
        new AutoKeyState("released", EventConstants.RELEASED);


    /** All states indexed by name */
    private static Hashtable keyStates; 

    /** State name */
    private String name;

    /** int value used for state by our MIDP implementation */
    private int midpKeyState;    


    /**
     * Gets state name.
     *
     * @return state name as string
     */
    public String getName() {
        return name;
    }

    /**
     * Gets int value used for state by our MIDP implementation.
     *
     * @return int value used for state by our MIDP implementation
     */
    int getMIDPKeyState() {
        return midpKeyState;
    }

    /**
     * Gets key state by name.
     *
     * @param name key state
     * @return AutoKeyState representing state with specified name
     */
    static AutoKeyState getByName(String name) {
        return (AutoKeyState)keyStates.get(name);
    }

    /**
     * Gets key state by corresponding MIDP key state.
     *
     * @param midpKeyState state constant used by our MIDP implementation
     * @return key state corresponding to the specified constant
     */
    static AutoKeyState getByMIDPKeyState(int midpKeyState) {
        Enumeration e = keyStates.elements();
        while (e.hasMoreElements()) {
            AutoKeyState keyState = (AutoKeyState)e.nextElement();
            if (keyState.getMIDPKeyState() == midpKeyState) {
                return keyState;
            }
        }

        return null;
    }

    /**
     * Private constructor to prevent creation of class instances.
     *
     * @param name key state name
     * @param midpKeyState int value used for state by our MIDP implementation
     */
    private AutoKeyState(String name, int midpKeyState) {
        this.name = name;
        this.midpKeyState = midpKeyState;

        if (keyStates == null) {
            keyStates = new Hashtable();
        }
        keyStates.put(name, this);
    }
}
