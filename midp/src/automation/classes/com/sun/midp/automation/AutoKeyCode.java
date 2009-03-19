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
import java.util.*;

/**
 * Represents key codes which aren't characters.
 */
public final class AutoKeyCode {
    /**
     * Key code: backspace key
     */
    public final static AutoKeyCode BACKSPACE = 
        new AutoKeyCode("backspace");

    /**
     * Key code: up key
     */
    public final static AutoKeyCode UP = 
        new AutoKeyCode("up");

    /**
     * Key code: down key
     */
    public final static AutoKeyCode DOWN = 
        new AutoKeyCode("down");

    /**
     * Key code: left key
     */
    public final static AutoKeyCode LEFT = 
        new AutoKeyCode("left");

    /**
     * Key code: right key
     */
    public final static AutoKeyCode RIGHT = 
        new AutoKeyCode("right");

    /**
     * Key code: select key
     */
    public final static AutoKeyCode SELECT = 
        new AutoKeyCode("select");

    /**
     * Key code: soft key 1 (left)
     */
    public final static AutoKeyCode SOFT1 = 
        new AutoKeyCode("soft1");

    /**
     * Key code: soft key 2 (right)
     */
    public final static AutoKeyCode SOFT2 = 
        new AutoKeyCode("soft2");

    /**
     * Key code: clear key
     */
    public final static AutoKeyCode CLEAR = 
        new AutoKeyCode("clear");

    /**
     * Key code: send key
     */
    public final static AutoKeyCode SEND = 
        new AutoKeyCode("send");

    /**
     * Key code: end key
     */
    public final static AutoKeyCode END = 
        new AutoKeyCode("end");
 
    /**
     * Key code: power key
     */
    public final static AutoKeyCode POWER = 
        new AutoKeyCode("power");

    /**
     * Key code: gamea key
     */
    public final static AutoKeyCode GAMEA = 
        new AutoKeyCode("gamea");

    /**
     * Key code: gameb key
     */
    public final static AutoKeyCode GAMEB = 
        new AutoKeyCode("gameb");

    /**
     * Key code: gamec key
     */
    public final static AutoKeyCode GAMEC = 
        new AutoKeyCode("gamec");

    /**
     * Key code: gamed key
     */
    public final static AutoKeyCode GAMED = 
        new AutoKeyCode("gamed");


    /** All key codes indexed by name */
    private static Hashtable keyCodes;

    /** Key code name */
    private String name;

    /** int value used for key code by our MIDP implemenation */
    private int midpKeyCode;    


    /**
     * Gets key code name
     *
     * @return key code name as string
     */
    public String getName() {
        return name;
    }


    /**
     * Gets key code by name.
     *
     * @param name key code name
     * @return key code with specified name
     */
    static AutoKeyCode getByName(String name) {
        return (AutoKeyCode)keyCodes.get(name);
    }

    /**
     * Gets key code by corresponding MIDP key code.
     *
     * @param midpKeyCode key code constant used by our MIDP implementation
     * @return key code corresponding to the specified constant
     */
    static AutoKeyCode getByMIDPKeyCode(int midpKeyCode) {
        Enumeration e = keyCodes.elements();
        while (e.hasMoreElements()) {
            AutoKeyCode keyCode = (AutoKeyCode)e.nextElement();
            if (keyCode.getMIDPKeyCode() == midpKeyCode) {
                return keyCode;
            }
        }

        return null;
    }    

    /**
     * Gets MIDP key code.
     *
     * @return int value used for key code by our MIDP implemenation
     */
    int getMIDPKeyCode() {
        return midpKeyCode;
    }

    /**
     * Gets MIDP key code for key code name.
     *
     * @param name key code name
     * @return int value used for key code by our MIDP implemenation
     */
    private static native int getMIDPKeyCodeForName(String name);

    /**
     * Private constructor to prevent creating class instances.
     *
     * @param name key code name
     */
    private AutoKeyCode(String name) {
        this.name = name;
        this.midpKeyCode = getMIDPKeyCodeForName(name);

        if (keyCodes == null) {
            keyCodes = new Hashtable();
        }
        keyCodes.put(name, this);
    }
}
