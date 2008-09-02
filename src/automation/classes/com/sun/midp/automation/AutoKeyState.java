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

public final class AutoKeyState {
    public final static AutoKeyState PRESSED = 
        new AutoKeyState("pressed", EventConstants.PRESSED);

    public final static AutoKeyState REPEATED = 
        new AutoKeyState("repeated", EventConstants.REPEATED);

    public final static AutoKeyState RELEASED = 
        new AutoKeyState("released", EventConstants.RELEASED);


    private final static Hashtable keyStates = new Hashtable();    

    private String name;
    private int midpKeyState;    


    public String getName() {
        return name;
    }

    int getMIDPKeyState() {
        return midpKeyState;
    }

    static AutoKeyState getByName(String name) {
        return (AutoKeyState)keyStates.get(name);
    }

    private AutoKeyState(String name, int midpKeyState) {
        this.name = name;
        this.midpKeyState = midpKeyState;

        keyStates.put(name, this);
    }
}
