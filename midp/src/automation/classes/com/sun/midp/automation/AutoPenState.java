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
 * Represents pen state.
 */
public final class AutoPenState {
    /**
     * State: pen tip down
     */
    public static final AutoPenState PRESSED = 
        new AutoPenState("pressed", EventConstants.PRESSED);

    /**
     * State: pen tip dragged
     */
    public static final AutoPenState DRAGGED = 
        new AutoPenState("dragged", EventConstants.DRAGGED);

    /**
     * State: pen tip released
     */
    public static final AutoPenState RELEASED = 
        new AutoPenState("released", EventConstants.RELEASED);


    /** All states indexed by name */
    private static Hashtable penStates;

    /** State name */
    private String name;

    /** int value used for state by our MIDP implementation */
    private int midpPenState;
    

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
    int getMIDPPenState() {
        return midpPenState;
    }

    /**
     * Gets pen state by name.
     *
     * @param name pen state
     * @return AutoPenState representing pen state with specified name
     */
    static AutoPenState getByName(String name) {
        return (AutoPenState)penStates.get(name);
    }

    /**
     * Gets pen state by corresponding MIDP key state.
     *
     * @param midpPenState state constant used by our MIDP implementation
     * @return pen state corresponding to the specified constant
     */
    static AutoPenState getByMIDPPenState(int midpPenState) {
        Enumeration e = penStates.elements();
        while (e.hasMoreElements()) {
            AutoPenState penState = (AutoPenState)e.nextElement();
            if (penState.getMIDPPenState() == midpPenState) {
                return penState;
            }
        }

        return null;
    }

    /**
     * Private constructor to prevent creation of class instances.
     *
     * @param name pen state name
     * @param midpPenState int value used for state by our MIDP implementation
     */
    private AutoPenState(String name, int midpPenState) {
        this.name = name;
        this.midpPenState = midpPenState;

        if (penStates == null) {
            penStates = new Hashtable();
        }
        penStates.put(name, this);
    }
}
