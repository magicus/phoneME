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
import com.sun.midp.events.*;
import com.sun.midp.lcdui.EventConstants;

/**
 * Implements AutoKeyEvent interface.
 */
final class AutoKeyEventImpl 
    extends AutoEventImplBase implements AutoKeyEvent {
    
    /** Constant for "code" argument name */
    static final String CODE_ARG_NAME = "code";

    /** Constant for "code" argument name */    
    static final String STATE_ARG_NAME = "state";

    /** Key code (null if key char is used instead) */
    private AutoKeyCode keyCode = null;

    /** Key char (if key code is used, then the value is unspecified) */
    private char keyChar;

    /** Key state */
    private AutoKeyState keyState = null;   


    /**
     * Gets key state.
     *
     * @return AutoKeyState representing key state
     */
    public AutoKeyState getKeyState() {
        return keyState;
    }  

    /**
     * Gets key code.
     *
     * @return AutoKeyCode representing key code if event has a key code, 
     * or null, if event has key char instead
     */
    public AutoKeyCode getKeyCode() {
        return keyCode;
    }


    /**
     * Gets key char.
     *
     * @return key char if event has it, or unspecified,
     * if event has key code instead
     */
    public char getKeyChar() {
        return keyChar;
    }


    /**
     * Gets string representation of event. The format is following:
     *  key code: code_value, state: state_value
     * where code_value and state_value are string representation 
     * of key code (key char) and key state. For example:
     *  key code: soft1, state: pressed
     *  key code: a, state: pressed
     */
    public String toString() {
        String typeStr = getType().getName();
        String stateStr = getKeyState().getName();

        String keyStr;
        if (keyCode != null) {
            keyStr = keyCode.getName();
        } else {
            char[] arr = { keyChar };
            keyStr = new String(arr);
        }

        String eventStr = typeStr + " " + CODE_ARG_NAME + ": " + keyStr + 
            ", " + STATE_ARG_NAME + ": " + stateStr;

        return eventStr;
    }

    /**
     * Constructor. Creates key event with a key code.
     *
     * @param keyCode key code
     * @param keyState key state
     */
    AutoKeyEventImpl(AutoKeyCode keyCode, AutoKeyState keyState) {
        super(AutoEventType.KEY);

        if (keyCode == null) {
            throw new IllegalArgumentException("Key code is null");
        }
        
        if (keyState == null) {
            throw new IllegalArgumentException("Key state is null");
        }
        
        this.keyCode = keyCode;
        this.keyState = keyState;        
    }

    /**
     * Gets native event (used by our MIDP implementation) 
     * corresponding to this Automation event.
     *
     * @return native event corresponding to this Automation event
     */
    NativeEvent toNativeEvent() {
        NativeEvent e = createNativeEvent();
        return e;
    }

    /**
     * Constructor. Creates key event with a key char.
     *
     * @param keyChar key char
     * @param keyState key state
     */
    AutoKeyEventImpl(char keyChar, AutoKeyState keyState) {
        super(AutoEventType.KEY);

        if (keyState == null) {
            throw new IllegalArgumentException("Key state is null");
        }

        this.keyCode = null;
        this.keyChar = keyChar;
        this.keyState = keyState;        
    }

    boolean isPowerButtonEvent() {
        return (keyCode == AutoKeyCode.POWER && keyState == AutoKeyState.RELEASED);
    }

    /**
     * Creates native event (used by our MIDP implementation) 
     * corresponding to this Automation event.
     * 
     * @return native event corresponding to this Automation event 
     */
    private NativeEvent createNativeEvent() {
        NativeEvent nativeEvent = new NativeEvent(EventTypes.KEY_EVENT);

        nativeEvent.intParam1 = keyState.getMIDPKeyState();
        if (keyCode != null) {
            nativeEvent.intParam2 = keyCode.getMIDPKeyCode();
        } else {
            nativeEvent.intParam2 = (int)keyChar;
        }

        return nativeEvent;
    }
}
