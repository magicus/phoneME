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

final class AutoKeyEventImpl 
    extends AutoEventImplBase implements AutoKeyEvent {

    private static Hashtable validKeyCodes = null;

    private AutoKeyCode keyCode = null;
    private char keyChar;
    private AutoKeyState keyState = null;   

    AutoKeyEventImpl(AutoKeyCode keyCode, AutoKeyState keyState) {
        super(AutoEventType.KEYBOARD, 
                createNativeEvent(keyState, keyCode, ' '));

        if (keyState == null || keyCode == null) {
            throw new IllegalArgumentException(
                    "Key code or key state is null");
        }

        this.keyCode = keyCode;
        this.keyState = keyState;        
    }

    AutoKeyEventImpl(char keyChar, AutoKeyState keyState) {
        super(AutoEventType.KEYBOARD, 
                createNativeEvent(keyState, null, keyChar));

        if (keyState == null) {
            throw new IllegalArgumentException(
                    "Key state is null");
        }

        this.keyChar = keyChar;
        this.keyState = keyState;        
    }    

    private AutoKeyEventImpl() {
        super();

        this.keyState = null;
        this.keyCode = null;
    }

    public AutoKeyState getKeyState() {
        return keyState;
    }  

    public AutoKeyCode getKeyCode() {
        return keyCode;
    }

    public char getKeyChar() {
        return keyChar;
    }

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

        String eventStr = typeStr + " " + stateStr + " " + keyStr;
        return eventStr;
    }

    static int registerKeyCode(AutoKeyCode keyCode) {
        if (validKeyCodes == null) {
            validKeyCodes = new Hashtable();
        }
        validKeyCodes.put(keyCode.getName(), keyCode);

        int midpKeyCode = getMIDPKeyCodeFromName(keyCode.getName());
        return midpKeyCode;
    }

    private static native int getMIDPKeyCodeFromName(String keyCodeName);

    private static NativeEvent createNativeEvent(AutoKeyState keyState, 
            AutoKeyCode keyCode, char  keyChar) {
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
